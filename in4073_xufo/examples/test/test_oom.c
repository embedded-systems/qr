
#ifndef IX86
#include <assert.h>
#include <x32.h>
#include <setjmp.h>

jmp_buf env;
int i;

static void go_oom();

static void oom_isr() {
	#ifdef PROCSTATE_REGISTER_INDEX
		assert(STATE_OUT_OF_MEMORY);
	#endif
	longjmp(env, 10);
}

static void go_oom() {
	int data[256];
	go_oom();
}

void test_oom() {
	INTERRUPT_VECTOR(INTERRUPT_OUT_OF_MEMORY) = &oom_isr;
	INTERRUPT_PRIORITY(INTERRUPT_OUT_OF_MEMORY) = 0xF003;
	ENABLE_INTERRUPT(INTERRUPT_OUT_OF_MEMORY);

	i = 0;
	if (!setjmp(env)) go_oom();
}

#else
void test_oom() {}
#endif
