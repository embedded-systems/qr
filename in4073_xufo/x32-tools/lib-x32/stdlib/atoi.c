#include <stdlib.h>

int atoi(const char* s) {
	return (int)strtol(s, (char**)NULL, 10);
}
