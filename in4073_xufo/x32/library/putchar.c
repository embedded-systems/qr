#include <x32.h>
#include <stdio.h>

int putchar(int chr) {
	while(!X32_STDOUT_STATUS);
	X32_STDOUT = chr;
	return chr;
}
