
#ifndef IX86
#include <assert.h>
#include <x32.h>

int counter;

static void breakpoint() {
	counter++;
}

static void trap() {
	INSTRUCTION *address;

	assert(counter==0);
	address = (INSTRUCTION*)&breakpoint;
	*address &= ~0x8000;
	counter++;
}


void test_trap() {
	INSTRUCTION *code;

	counter = 0;

	INTERRUPT_VECTOR(INTERRUPT_TRAP) = &trap;
	INTERRUPT_PRIORITY(INTERRUPT_TRAP) = 10;
	ENABLE_INTERRUPT(INTERRUPT_TRAP);

	code = (INSTRUCTION*)&breakpoint;
	*code |= 0x8000;

	breakpoint();

	assert(counter==2);
	DISABLE_INTERRUPT(INTERRUPT_TRAP);
}



#else
int test_trap() {}
#endif
