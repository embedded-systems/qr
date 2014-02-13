#include <string.h>

size_t strlen(const char *cs) {
	size_t i = 0;
	char *str = (char*)cs;

	while(*(str++)) i++;
	return i;
}
