#include <stdio.h>

static int _pc(int chr, void* param) {
	putchar(chr);
	return 1; 
}


int printf(const char* fmt, ...) {
	va_list ap;
	int ret;

	/* start reading arguments */
	va_start(ap, fmt);

	ret = _xprintf(_pc, 0, fmt, &ap);

	/* stop reading parameters */
	va_end(ap);

	return ret;
}

int vprintf(const char* fmt, va_list arg) {
	return _xprintf(_pc, 0, fmt, &arg);
}
