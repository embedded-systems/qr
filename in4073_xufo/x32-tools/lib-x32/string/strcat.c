#include <string.h>

char* strcat(char *s, const char *ct) {
	char *dest = s;
	char *src = (char*)ct;

	while (*dest) dest++;
	while (*src) {
		*dest = *src;
		dest++; src++;
	}
	*dest = '\0';

	return s;
}
