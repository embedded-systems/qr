#include <string.h>

size_t strspn(const char *cs, const char *ct) {
	char *t = (char*)ct;
	int i = 0;

	while(cs[i] && *t) {
		while(*t) {
			if (*t == cs[i]) {
				i++;
				t = (char*)ct;
			} else {
				t++;
			}
		}
	}

	return i;
}

