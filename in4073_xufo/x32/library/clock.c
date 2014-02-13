#include <x32.h>

clock_t clock() {
	return X32_US_CLOCK;
}
