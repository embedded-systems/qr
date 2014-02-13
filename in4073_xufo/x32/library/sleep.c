#include <x32.h>

void sleep(unsigned ms) {
	unsigned start = X32_MS_CLOCK;
	while(X32_MS_CLOCK-start>ms);
}

void usleep(unsigned us) {
	unsigned start = X32_US_CLOCK;
	while(X32_US_CLOCK-start>us);
}
