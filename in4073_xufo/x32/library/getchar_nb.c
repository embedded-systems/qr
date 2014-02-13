#include <x32.h>
#include <stdio.h>

int getchar_nb() {
	if (X32_STDIN_STATUS) {
		return X32_STDIN;
	} else {
		return -1;
	}
}
