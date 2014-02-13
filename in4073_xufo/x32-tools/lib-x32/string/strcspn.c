#include <string.h>

size_t strcspn(const char *cs, const char *ct) {
	char *t;
	int i = 0;

	while(cs[i]) {
		t = (char*)ct;
		while(*t) if (*t++ == cs[i]) return i;
		i++;
	}

	return i;
}

