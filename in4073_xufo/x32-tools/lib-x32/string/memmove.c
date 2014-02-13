#include <string.h>

void *memmove(void *dest, const void *src, size_t num) {
	int i;
	
	if (dest < src) {
		for (i = 0; i < num; i++) ((char*)dest)[i] = ((char*)src)[i];
	} else if (dest > src) {
		for (i = num-1; i >= 0; i--) ((char*)dest)[i] = ((char*)src)[i];
	}

	return dest;
}

