#include <string.h>

static char* ptr;

char *strtok(char* s, const char* ct) {
	int i;

	if (s == 0) s = ptr;
	
	s += strspn(s, ct);

	if (*s == 0) return NULL;

	if (i = strcspn(s, ct)) s[i] = 0;
	ptr = &s[i+1];

	return s;
}


