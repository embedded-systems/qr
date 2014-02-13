#include <x32.h>
#include <stdio.h>
#include <setjmp.h>

void some_function(void);
void handle_breakpoint(void);

INSTRUCTION old_instruction;

/*
 */

int main(int argc, char** argv) {
	/* set breakpoint handler: */
	SET_INTERRUPT_VECTOR(INTERRUPT_TRAP, &handle_breakpoint);
	/* note: priority of breakpoint handler should be very high! */
	SET_INTERRUPT_PRIORITY(INTERRUPT_TRAP, 10000);
	/* enable interrupts */
	ENABLE_INTERRUPT(INTERRUPT_TRAP);
	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);

	/* set the breakpoint:
	 *		overwrite the first instruction in some_function with 0x1000, 
	 *		the trap instruction
	 */
	old_instruction = *((INSTRUCTION*)&some_function);
	printf("INIT: Writing trap instruction to 0x%p (old value: 0x%04X)\r\n", &some_function, old_instruction);
	*((unsigned short*)&some_function) = 0x8000;
	printf("INIT: Instruction at 0x%p: 0x%04X\r\n", &some_function, *((INSTRUCTION*)&some_function));

	/* call the function */
	some_function();
	
	/* disable interrupts before exiting */
	DISABLE_INTERRUPT(INTERRUPT_GLOBAL);
	DISABLE_INTERRUPT(INTERRUPT_TRAP);
	return 10;
}

/* some function to test with */
void some_function(void) {
	/* do stuff here */
	printf("In some function\r\n");
}

/* breakpoint handler function */
void handle_breakpoint(void) {
	unsigned long* ra;

	printf("TRAP: Trapped!\r\n");

	ra = (unsigned long*)((unsigned char*)_get_fp()-5*sizeof(int*));

	printf("TRAP: Restoring instruction 0x%04X to 0x%04X\r\n", old_instruction, *ra-2);
	//*((INSTRUCTION*)(*ra-2)) = old_instruction;
	*((INSTRUCTION*)(*ra)) = old_instruction;
	printf("TRAP: Instruction at 0x%p: 0x%04X\r\n", &some_function, *((INSTRUCTION*)&some_function));
	printf("TRAP: Restoring return address\r\n");
	//*ra = *ra-2;
	printf("Returning to 0x%04X\r\n", *ra);
}
