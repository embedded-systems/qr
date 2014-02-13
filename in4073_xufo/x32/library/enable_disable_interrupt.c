#include <x32.h>

#ifdef PERIPHERAL_INT_ENABLE
void enable_interrupt(int index) {
	set_execution_level(THREAD_SAFE_EXECUTION_LEVEL);
	peripherals[PERIPHERAL_INT_ENABLE] |= (1<<index);
}

void disable_interrupt(int index) {
	set_execution_level(THREAD_SAFE_EXECUTION_LEVEL);
	peripherals[PERIPHERAL_INT_ENABLE] &= ~(1<<index);
}
#endif // PERIPHERAL_INT_ENABLE
