#include <stdlib.h>

long atol(const char* s) {
	return strtol(s, (char**)NULL, 10);
}
