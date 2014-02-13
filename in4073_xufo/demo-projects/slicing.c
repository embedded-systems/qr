/*------------------------------------------------------------------
 *  slicing.c -- demo context switching on X32
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include "x32.h"

/* define some peripheral short hands
 */
#define X32_buttons	peripherals[PERIPHERAL_BUTTONS]
#define X32_clock	peripherals[PERIPHERAL_MS_CLOCK]
#define X32_timer_per	peripherals[PERIPHERAL_TIMER1_PERIOD]

int	demo_done;
int	thread_id;
void	*stack[1024]; 		// stack for task1
void	**task0_context;	// store context for task0
void	**task1_context;	// store context for task1

/*------------------------------------------------------------------
 * delay -- busy-wait for ms milliseconds
 *------------------------------------------------------------------
 */
void delay(int ms) 
{
	int time = X32_clock;
	while(X32_clock - time < ms)
		;
}

/*------------------------------------------------------------------
 * isr_buttons -- buttons interrupt handler
 *------------------------------------------------------------------
 */
void isr_buttons(void)
{
	printf("%d: button pressed\r\n",X32_clock);
	if (X32_buttons == 0x09)
		demo_done = 1;
}

/*------------------------------------------------------------------
 * isr_timer -- timer interrupt handler
 *------------------------------------------------------------------
 */
void isr_timer(void)
{
	if (thread_id == 0) {
		thread_id = 1;
		context_switch(task1_context,&task0_context);
	}
	else {
		thread_id = 0;
		context_switch(task0_context,&task1_context);
	}
}

/*------------------------------------------------------------------
 * task1 -- task1
 *------------------------------------------------------------------
 */
void 	task1() 
{
	while (1) {
		printf("%d: task thread (id = %d)\r\n",X32_clock,thread_id);
		delay(100);
	}
}

/*------------------------------------------------------------------
 * main -- do the demo
 *------------------------------------------------------------------
 */
int 	main() 
{
	/* prepare buttons
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_BUTTONS, &isr_buttons);
        SET_INTERRUPT_PRIORITY(INTERRUPT_BUTTONS, 10);
        ENABLE_INTERRUPT(INTERRUPT_BUTTONS);

	/* prepare timer interrupt every 20 ms
	 */
	X32_timer_per = 20 * CLOCKS_PER_MS;
        SET_INTERRUPT_VECTOR(INTERRUPT_TIMER1, &isr_timer);
        SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER1, 11);
        ENABLE_INTERRUPT(INTERRUPT_TIMER1);

	/* initialize a context for task1 
	 * (task0 (main) already has a context (it's running)
	 */
	task1_context = init_stack(stack, (void *) task1, (void *) 0);

	/* register main as current thread
	 */
	thread_id = 0;

	/* start the fireworks
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	demo_done = 0;
	while (1) {
		printf("%d: main thread (id = %d)\r\n",X32_clock,thread_id);
		// delay(100);
		if (demo_done) break;
	}

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	return 0;
}






