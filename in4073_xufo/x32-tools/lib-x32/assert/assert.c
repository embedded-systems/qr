#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int _assert(char *expression, char *file, unsigned line) {
	printf("Assertion failed: %s, file %s, line %d\r\n",
		expression, file, line);
	abort();
	return 0;
}
