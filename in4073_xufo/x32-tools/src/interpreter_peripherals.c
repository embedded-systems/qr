#include "interpreter_peripherals.h"
/*
 * Interpreter peripheral simulator, change this file to modify the 
 *  peripheral set for the bytecode interpreter.
 *
 *  to add a writable device: 
 *  	add code to set_peripheral_value
 *	to add a readable device:
 *		add code to get_peripheral_value
 *	to trigger an interrupt:
 *		set the 'pending' field of the specific interrupt to 1
 *		(for example, to raise irq 3:	int_vector[3].pending = 1;)
 *		
 *		the check_interrupts function is called before each instruction,
 *		and can thus be used as polling function to check if an interrupt
 *		should trigger.
 */

/* peripheral registers */
unsigned timer1_period = 0;		/* timer 1 period (in clockcycles) */
unsigned timer1 = 0;					/* timer 1 value */
unsigned timer2_period = 0;		/* timer 2 period (in clockcycles) */
unsigned timer2 = 0;					/* timer 2 value */
unsigned int_enable = 0;			/* interrupt enable register */
unsigned display = 0;					/* display register */
unsigned buttons = 0;					/* buttons register */
unsigned switches = 0;				/* switches register */
unsigned leds = 0;						/* led register */
unsigned procstate = 3;				/* processor state register (init 1, booting) */

unsigned stdout_cd = 0;				/* stdout cooldown instructions */

int com1_buff = -1;			/* 1 byte buffer for COM1, this buffer is used to
														chech whether a byte is available (COM1_STATE).
														when its empty/nonvalid, its value is set to -1,
														otherwise it contains a valid byte. when reading
														a character the buffer should first be checked,
														if its empty/nonvalid, a byte may be read from
														the actual system buffer
												*/

#define INTERRUPT_COUNT INTERRUPT_MAX+1

typedef struct {
	int address;
	int priority;
	int pending;
} InterruptDescriptor;

InterruptDescriptor int_vector[INTERRUPT_COUNT];

void check_interrupts(BOOL, BOOL, BOOL, BOOL);

int init_peripherals() {
	int i;
	
	/* initialize console I/O */
	if (!init_io()) {
		printf("Can't initialize I/O\n");
		return 0;
	}
	
	/* clear interrupt registers */
	for (i = 0; i < INTERRUPT_COUNT; i++) {
		int_vector[i].address = 0;
		int_vector[i].priority = 0;
	}
}

void term_peripherals() {
	/* terminate console I/O */
	term_io();
}

/* the interpreter calls this value to read from a peripheral device.
		the index parameter holds the index of the peripheral device
		(memory address = 0x80000000 + index * 4). The device should return
		a 32-bit value created with create_int() or create_unsigned().
*/
Value* get_peripheral_value(Interpreter* interpreter, int index) {
	Value* tmp; 
	int int_index;

	switch(index) {
#ifdef PERIPHERAL_UID
		case PERIPHERAL_UID:
			return create_int(PERIPHERAL_VERSION);
#endif // PERIPHERAL_UID
#ifdef PERIPHERAL_PRIMARY_DATA
		case PERIPHERAL_PRIMARY_DATA:
			/* check if there's a byte in the buffer */
			if (com1_buff >= 0) {
				/* invalid the buffer, and return it */
				tmp = create_int(com1_buff);
				com1_buff = -1;
				return tmp;
			} else {
				/* return a new value */
				return create_int(read_char_non_blocking());
			}
#endif
#ifdef PERIPHERAL_PRIMARY_STATUS
		case PERIPHERAL_PRIMARY_STATUS:
			/* try to read a value into the buffer if its empty */
			if (com1_buff < 0) com1_buff = read_char_non_blocking();
			/* return the state */
			return create_int((stdout_cd==0?1:0) + (com1_buff>=0?2:0));
#endif
#ifdef PERIPHERAL_INSTRCNTR
		case PERIPHERAL_INSTRCNTR:
			return create_int(interpreter->executed);
#endif 
#ifdef PERIPHERAL_MS_COUNTER
		case PERIPHERAL_MS_COUNTER:
			return create_int(interpreter->executed / INSTRCTIONS_PER_MS);
#endif 
#ifdef PERIPHERAL_DISPLAY
		case PERIPHERAL_DISPLAY:
			return create_int(display);
#endif
#ifdef PERIPHERAL_SWITCHES
		case PERIPHERAL_SWITCHES:
			return create_int(switches);
#endif
#ifdef PERIPHERAL_LEDS
		case PERIPHERAL_LEDS:
			return create_int(leds);
#endif
#ifdef PERIPHERAL_BUTTONS
		case PERIPHERAL_BUTTONS:
			return create_int(buttons);
#endif
#ifdef PERIPHERAL_SECONDARY_DATA
		case PERIPHERAL_SECONDARY_DATA:
			/* no support for COM2, could be attached to serial device! */
			return create_int(-1);
#endif
#ifdef PERIPHERAL_SECONDARY_STATUS
		case PERIPHERAL_SECONDARY_STATUS:
			/* no support for COM2, could be attached to serial device! */
			return create_int(0);
#endif
#ifdef PERIPHERAL_INTERRUPT_ENABLE
		case PERIPHERAL_INTERRUPT_ENABLE:
			return create_int(int_enable);
#endif
#ifdef PERIPHERAL_TIMER1_PERIOD
		case PERIPHERAL_TIMER1_PERIOD:
			return create_int(timer1_period);
#endif
#ifdef PERIPHERAL_TIMER2_PERIOD
		case PERIPHERAL_TIMER2_PERIOD:
			return create_int(timer2_period);
#endif
#ifdef PERIPHERAL_PROCSTATE
		case PERIPHERAL_PROCSTATE:
			/* reset procstate on read, use int_index as temp variable */
			int_index = procstate;
			procstate = 2;
			return create_int(int_index);
#endif
		default:
#ifdef PERIPHERAL_INT_VECT_BASE			
			int_index = index - PERIPHERAL_INT_VECT_BASE;
			if (index >= 0 && index < 2*INTERRUPT_COUNT) {
				if (index & 0x01) {
					return create_int(int_vector[index>>1].priority);
				} else {
					return create_int(int_vector[index>>1].address);
				}
			}
#endif
			return 0;
	}
}

/* the interpreter calls this value to write to a peripheral device.
		the index parameter holds the index of the peripheral device
		(memory address = 0x80000000 + index * 4). The value to write
		is hold in the value structure, but the 'realval' integer can
		also be used (this holds the value in 32-bit intel format).
*/
/* set a peripheral value, only the writable peripherals are included here */
void set_peripheral_value(Interpreter* interpreter, int index, Value* value) {
	int realval = parse_int(value);
	int int_index;

	switch(index) {
#ifdef PERIPHERAL_PRIMARY_DATA
		case PERIPHERAL_PRIMARY_DATA:
			write_char((char)realval);
			stdout_cd = STDOUT_COOLDOWN;
#ifdef INTERRUPT_PRIMARY_TX
			/* set this interrupt on pending */
			int_vector[INTERRUPT_PRIMARY_TX].pending = 1;
#endif
			break;
#endif
#ifdef PERIPHERAL_DISPLAY
		case PERIPHERAL_DISPLAY:
			display = realval;
			break;
#endif
#ifdef PERIPHERAL_LEDS
		case PERIPHERAL_LEDS:
			leds = realval;
			break;
#endif
#ifdef PERIPHERAL_SECONDARY_DATA
		case PERIPHERAL_SECONDARY_DATA:
			/* no support for COM2 */
			break;
#endif
#ifdef PERIPHERAL_INTERRUPT_ENABLE
		case PERIPHERAL_INTERRUPT_ENABLE:
			//if ((int_enable & 0x2000) != (realval & 0x2000)) printf("INT: %s\r\n", (realval & 0x2000)?"ON":"OFF");
			int_enable = realval;
			break;
#endif
#ifdef PERIPHERAL_TIMER1_PERIOD
		case PERIPHERAL_TIMER1_PERIOD:
			timer1_period = realval;
			break;
#endif
#ifdef PERIPHERAL_TIMER2_PERIOD
		case PERIPHERAL_TIMER2_PERIOD:
			timer2_period = realval;
			break;
#endif
#ifdef PERIPHERAL_SOFTINT1
	#ifdef INTERRUPT_SOFTINT1
		case PERIPHERAL_SOFTINT1:
			int_vector[INTERRUPT_SOFTINT1].pending = 1;
			break;
	#endif
#endif
#ifdef PERIPHERAL_SOFTINT2
	#ifdef INTERRUPT_SOFTINT2
		case PERIPHERAL_SOFTINT2:
			int_vector[INTERRUPT_SOFTINT1].pending = 1;
			break;
	#endif
#endif
#ifdef PERIPHERAL_SOFTINT3
	#ifdef INTERRUPT_SOFTINT3
		case PERIPHERAL_SOFTINT3:
			int_vector[INTERRUPT_SOFTINT3].pending = 1;
			break;
	#endif
#endif
#ifdef PERIPHERAL_SOFTINT4
	#ifdef INTERRUPT_SOFTINT4
		case PERIPHERAL_SOFTINT4:
			int_vector[INTERRUPT_SOFTINT4].pending = 1;
			break;
	#endif
#endif
		default:
#ifdef PERIPHERAL_INT_VECT_BASE			
			int_index = index - PERIPHERAL_INT_VECT_BASE;
			if (int_index >= 0 && int_index < 2*INTERRUPT_COUNT) {
				if (int_index & 0x01) {
					int_vector[int_index>>1].priority = realval;
				} else {
					int_vector[int_index>>1].address = realval;
				}
			}
#endif
			break;
	}
}

/* this function is called from interrupt_pending, which is called
			before each instruction. It should fill the 'pending' field
			for each interrupt device.
*/
void check_interrupts(BOOL trapped, BOOL overflow, BOOL div0, BOOL oom) {

	if (overflow) procstate |= 0x10;
	if (div0) procstate |= 0x08;
	if (oom) procstate |= 0x40;

	if (stdout_cd) stdout_cd--;

#ifdef INTERRUPT_TIMER1
	/* check timer 1 */
	if (timer1_period) {
		timer1 += CLOCKS_PER_MS/INSTRCTIONS_PER_MS;
		if (timer1 > timer1_period) {
			int_vector[INTERRUPT_TIMER1].pending = 1;
			timer1 = 0;
		}
	}
#endif
#ifdef INTERRUPT_TIMER2
	/* check timer 2 */
	if (timer2_period) {
		timer2 += CLOCKS_PER_MS/INSTRCTIONS_PER_MS; 
		if (timer2 > timer2_period) {
			int_vector[INTERRUPT_TIMER2].pending = 1;
			timer2 = 0;
		}
	}
#endif
#ifdef INTERRUPT_PRIMARY_RX
	/* try to read a value into the buffer if its empty */
	if (com1_buff < 0) com1_buff = read_char_non_blocking();
	if (com1_buff >= 0) int_vector[INTERRUPT_PRIMARY_RX].pending = 1;
#endif
#ifdef INTERRUPT_TRAP
	if (trapped) int_vector[INTERRUPT_TRAP].pending = 1;
#endif
#ifdef INTERRUPT_OVERFLOW
	if (overflow) int_vector[INTERRUPT_OVERFLOW].pending = 1;
#endif
#ifdef INTERRUPT_DIVISION_BY_ZERO
	if (div0) int_vector[INTERRUPT_DIVISION_BY_ZERO].pending = 1;
#endif
#ifdef INTERRUPT_OUT_OF_MEMORY
	if (oom) int_vector[INTERRUPT_OUT_OF_MEMORY].pending = 1;
#endif

}

int interrupt_pending(unsigned* address, unsigned* priority, unsigned current_priority, 
	BOOL trapped, BOOL overflow, BOOL div0, BOOL oom) {

	
	InterruptDescriptor* highest = 0;
	int i;
	/* this function is called before each instruction */
	
	/* update the pending fields in the interrupt structures */
	check_interrupts(trapped, overflow, div0, oom);

	/* check all interrupts, get highest priority pending, the priority must
			 be higher than current_priority, and it must be enabled */
	for (i = 0; i < INTERRUPT_COUNT; i++) {
		/* UPDATE: Interrupts may not start pending when they are disabled */
		if (!(int_enable & (1<<i))) int_vector[i].pending = 0;

		if (int_vector[i].pending && int_vector[i].priority > current_priority &&  
					(highest == 0 || highest->priority < int_vector[i].priority) &&
					(int_enable & (1<<i)))  {
			
			if ((i < INTERRUPT_CRITICAL) || (int_enable & (1 << INTERRUPT_GLOBAL))) {
				highest = &int_vector[i];
			}
	}
	}
	
	if (highest) {
		/* return address & priority of new interrupt */
		*address = highest->address;
		*priority = highest->priority;
		/* no longer pending */
		highest->pending = 0;
		return 1;
	} else {
		return 0;
	}
}

