#include <x32.h>
#include <setjmp.h>

#define SOFTWARE_INTERRUPT_PRIORITY 10
#define PRIO_HIGHEST 0xFFFF

int raise_interrupt(int code, void* data);
int interrupt_handler(int code, void* data);
void combine_stackframe();

/*
 * Software interrupt demo
 *
 */
int main() {
	/* initialize software interrupt to the highest possible priority, to make 
			sure NO other interrupts get priority over the software interrupt (which
			causes an invalid stackframe) */
	SET_INTERRUPT_PRIORITY(INTERRUPT_SOFTINT1, PRIO_HIGHEST);
	SET_INTERRUPT_VECTOR(INTERRUPT_SOFTINT1, (void*)&interrupt_handler);

	ENABLE_INTERRUPT(INTERRUPT_SOFTINT1);
	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);
	
	/* the interrupt handle returns code + 1 (11 in this situation) */
	printf("INT 10: %d\r\n", raise_interrupt(10, 0));
	return 0;
}

int raise_interrupt(int code, void* data) {
	/* first, switch to highest execution level-1, to make sure hardware 
			interrupts are disabled (but not the software interrupt) */
	set_execution_level(PRIO_HIGHEST-1);

	/* now, raise the interrupt */
	peripherals[PERIPHERAL_SOFTINT1] = 1;

	/* the code here will never be executed, since a return of the software
			interrupt handler will be like a return from this function */
	printf("THIS LINE SHOULD NOT BE PRINTED!!\r\n");
	return -1;
}

int interrupt_handler(int code, void* data) {
	/* at this point, the we're still in a void(void) function! */
	combine_stackframe();
	/* at this point, we're in the int raise_interrupt(int, void*) function! */
	/* set the execution level which is wanted for this function to re-enable
			other interrupts (not required): */
	set_execution_level(SOFTWARE_INTERRUPT_PRIORITY);

	printf("In handler, code: %d, data: %p\r\n", code, data);	

	/* return from raise_interrupt (back to main) */
	return code + 1;
}

