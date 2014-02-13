#include <x32.h>
#include <stdio.h>

int getchar() {	
	int chr;
	while(!X32_STDIN_STATUS);
	chr = X32_STDIN;
	return chr;
}
