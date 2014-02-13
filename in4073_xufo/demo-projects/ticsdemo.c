#include <tics.h>
#include <ticsext.h>
#include "x32.h"

/* Reserve room for 40 messages... */
#define NUM_MSGS  40
/* ... and 20 tasks */
#define NUM_TASKS 20

/* Allocate space for the messages */
typeMsg MsgSpace[NUM_MSGS];
/* Allocate space for the task control blocks */
typeTcb TcbSpace[NUM_TASKS];
/* Allocate space for the task stacks */
unsigned char StackSpace[NUM_TASKS*DEF_STACK_SIZE_IN_BYTES];

/* Some shorthands for the X32 peripherals */
#define X32_display	 peripherals[PERIPHERAL_DISPLAY]
#define X32_leds	 peripherals[PERIPHERAL_LEDS]
#define X32_buttons	 peripherals[PERIPHERAL_BUTTONS]
#define X32_switches peripherals[PERIPHERAL_SWITCHES]

/* Message types */
#define MSG_BUTTON  1000
#define MSG_TICK    1001

/* 
 * The tcb for the display task is stored globally, so that other tasks/ISRs
 * can access it, to send messages.
 */
typeTcb *displayTask;

/*
 * The display task processes the incoming 'data', in this case button presses,
 * and displays it using the LEDs, and using serial communication.
 */
void display(int unused) {
    typeMsg *m;
    while(1) {
        /* 
         * Wait for a message. The waitMsg function blocks, until a message
         * arrives with a type matching the argument. Use the constant
         * ANY_MSG to receive messages regardless of the type.
         * The rcvMsg is the non-blocking variant of this function. This
         * function returns NULL if the desired message is not found in the
         * message queue.
         */
        m = waitMsg(ANY_MSG);
        switch(m->msgNum) {
            case MSG_BUTTON:
                printf("Buttons pressed: %d\r\n", m->iData);
                if(m->iData == 8) {
                    printf("Leaving tics...\r\n");
                    exitTics();
                }
                X32_leds = m->iData;
                break;
            case MSG_TICK:
                if(X32_switches & 128) {
                    X32_display -= (X32_switches & 127);
                } else {
                    X32_display += (X32_switches & 127);
                }
                break;
            default:
                printf("Unknown message type %d\r\n", m->msgNum);
                break;
        }
        /*
         * After a message has been received and processed, it should be freed
         * using the freeMsg function. This adds the message back to the free
         * message pool.
         */
        freeMsg(m);
    }
}

/*
 * A sample interrupt service routine, servicing the button interrupt. At the
 * start of every ISR, the isrEnter function should be called, telling the
 * kernel that the current task should not be preempted. When done, call the
 * function isrRet, which performs a context switch if messages have been sent.
 */
void buttonmonitor(void) {
    typeMsg *m;
    isrEnter();
    /* Create a new message, to displayTask, of type MSG_BUTTON */
    m = makeMsg(displayTask, MSG_BUTTON);
    /* 
     * Store integer data in the iData field. The lData field can be used for
     * longs, the pData field can be used for pointers. Do not put a pointer
     * to a local variable in this field, since the stack of the ISR is not
     * available to the receiver....
     */
    m->iData = peripherals[PERIPHERAL_BUTTONS];
    sendMsg(m);
    isrRet();
}

/*
 * Another ISR, which sends a message to the display task with a regular
 * interval. Note that a periodic timer could have been used as well, these
 * are provided by Tics.
 */
void timer2(void) {
    isrEnter();
    /* This message doesn't contain data */
    sendMsg(makeMsg(displayTask, MSG_TICK));
    isrRet();
}

/*
 * Tics is initialized in the main function. Tasks can be started here as
 * well.
 */
void main(void) {
    typeTics * tics;

    /*
     * makeTics does all the initialization work. Pointers to the various
     * memory spaces allocated at the top of this file are passed, along with
     * their sizes.
     */
    tics = makeTics(MsgSpace, sizeof(MsgSpace), StackSpace, sizeof(StackSpace), 
    TcbSpace, sizeof(TcbSpace)); 

    /* Start Tics */
    startTics(tics);

    /*
     * Create a task used for displaying the data. A pointer to the function
     * containing the task is passed, and the number of the task.
     */
    displayTask = makeTask(display, 0);
    /* Start the task */
    startTask(displayTask);

    /*
     * Initialize the ISR for the buttons. This is done in the usual way,
     * there is no specific Tics code needed for this. Make sure that the
     * priority is higher than the timer interrupt used by Tics (i.e. higher
     * than 1).
     */
    SET_INTERRUPT_VECTOR(INTERRUPT_BUTTONS, &buttonmonitor);
    SET_INTERRUPT_PRIORITY(INTERRUPT_BUTTONS, 30);
    ENABLE_INTERRUPT(INTERRUPT_BUTTONS);

    peripherals[PERIPHERAL_TIMER2_PERIOD] = 125 * CLOCKS_PER_MS;
    SET_INTERRUPT_VECTOR(INTERRUPT_TIMER2, &timer2);
    SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER2, 20);
    ENABLE_INTERRUPT(INTERRUPT_TIMER2);

    /*
     * Call suspend, which switches to the display task, since it is the only
     * runnable task. main never returns.
     */
    suspend();
}

