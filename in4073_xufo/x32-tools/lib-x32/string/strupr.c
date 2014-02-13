#include <string.h>
#include <ctype.h>

char *strupr(char *str) {
	char *s = str;
	while(*s) {
		if (islower(*s)) *s = *s - 'a' + 'A';
		s++;
	}
	return str;
}
