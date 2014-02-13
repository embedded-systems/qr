#include <stdlib.h>

div_t div(int num, int denom) {
	div_t ret;
	ret.quot = num/denom;
	ret.rem = num%denom;
	return ret;
}
