#include <string.h>

char *strcpy(char* s, const char* ct) {
	char *src = (char*)ct;

	while(*src != '\0') *(s++) = *(src++);
	*s = '\0';
	return s;
}
