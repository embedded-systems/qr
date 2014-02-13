#include <stdlib.h>

unsigned int holdrand = 1;

void srand(unsigned int seed) {
  holdrand = seed;
}

int rand() {
  return (holdrand = holdrand * 214013 + 2531011) & 0x7FFFFFFF;
}
