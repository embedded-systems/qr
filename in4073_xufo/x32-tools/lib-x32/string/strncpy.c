#include <string.h>

char *strncpy(char* s, const char* ct, int n) {
	char *src = (char*)ct;

	while(*src != '\0' && n-- > 0) *(s++) = *(src++);	
	while(n-- > 0) *s++ = '\0';
	return s;
}
