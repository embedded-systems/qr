#include <string.h>

char *strrchr(const char *cs, int c) {
	char *s = (char*)cs;
	char *ret = NULL;

	while(*s) {
		if (*s == c) ret = s;
		s++;
	}

	return ret;
}
