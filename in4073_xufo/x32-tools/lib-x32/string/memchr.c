#include <string.h>

void *memchr(const void* buffer, int c, size_t num) {
	int i;
	for (i = 0; i < num; i++) {
		if (((char*)buffer)[i] == c) return &((char*)buffer)[i];
	}
	return NULL;
}
