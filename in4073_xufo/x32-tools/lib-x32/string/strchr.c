#include <string.h>

char *strchr(const char *cs, int c) {
	char *s = (char*)cs;
	while(*s) {
		if (*s == c) return s;
		s++;
	}
	return NULL;
}
