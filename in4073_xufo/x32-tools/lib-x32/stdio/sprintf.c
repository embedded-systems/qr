#include <stdio.h>

static int _pc(int chr, void* param) {
	**(char**)param = (char)chr;
	*((char**)param) = *((char**)param)+1;
	return 1; 
}


int sprintf(char* buffer, const char* fmt, ...) {
	va_list ap;
	int ret;
	void *vbuffer = (void*)buffer;

	/* start reading arguments */
	va_start(ap, fmt);

	ret = _xprintf(_pc, (void*)&vbuffer, fmt, &ap);
	buffer[ret] = '\0';

	/* stop reading parameters */
	va_end(ap);

	return ret;
}

int vsprintf(char* buffer, const char* fmt, va_list arg) {
	void *vbuffer = (void*)buffer;
	return _xprintf(_pc, (void*)&vbuffer, fmt, &arg);
}
