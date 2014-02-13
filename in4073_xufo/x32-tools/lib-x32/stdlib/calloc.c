#include <stdlib.h>

void *calloc(size_t nobj, size_t size) {
	size_t total_size = nobj*size;
	size_t i;
	void *data = malloc(total_size);
	
	if (data) {
		for (i = 0; i < (total_size >> 2); i++) {
			((unsigned int*)data)[i] = 0;
		}
		((unsigned char*)data)[total_size-1] = 0;
		((unsigned char*)data)[total_size-2] = 0;
		((unsigned char*)data)[total_size-3] = 0;
	}
	return data;
}
