#include <string.h>

void *memset(void *buffer, int c, size_t num) {
	int i;

	for (i = 0; i < num; i++) ((char*)buffer)[i] = c;		

	return buffer;
}
