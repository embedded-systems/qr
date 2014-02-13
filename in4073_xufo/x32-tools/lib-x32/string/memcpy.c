#include <string.h>

void *memcpy(void *dest, const void *src, size_t num) {
	int i;

	for (i = 0; i < num; i++) ((char*)dest)[i] = ((char*)src)[i];

	return dest;
}
