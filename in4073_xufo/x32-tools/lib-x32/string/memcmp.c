#include <string.h>

int memcmp(const void *buffer1, const void *buffer2, size_t num) {
	int i;
	for (i = 0; i < num; i++) {
		if (((char*)buffer1)[i] < ((char*)buffer2)[i]) {
			return -1;
		} else if (((char*)buffer1)[i] > ((char*)buffer2)[i]) {
			return 1;
		} 
	}
	return 0;
}
