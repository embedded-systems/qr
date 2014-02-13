#include <string.h>
#include <ctype.h>

char *strlwr(char *str) {
	char *s = str;
	while(*s) {
		if (isupper(*s)) *s = *s - 'A' + 'a';
		s++;
	}
	return str;
}
