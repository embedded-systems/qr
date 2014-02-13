#include <stdlib.h>

ldiv_t ldiv(long num, long denom) {
	ldiv_t ret;
	ret.quot = num/denom;
	ret.rem = num%denom;
	return ret;
}
