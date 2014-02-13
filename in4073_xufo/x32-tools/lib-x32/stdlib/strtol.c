#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

long strtol(const char* s, char** endp, int base) {
	char *str = (char*)s;
	int v;
	long ret = 0;
	long prev_ret = 0;
	int sign = 0;

	while (*str == ' ' || *str == '\t') str++;

	if (*str == '-') {
		sign = 1;
		str++;
	} else if (*str == '+') {
		str++;
	}

	if (base == 0) {
		if (str[0] == '0') {
			if (str[1] == 'x' || str[1] == 'X') {
				str+=2;
				base = 16;
			} else {
				str+=1;
				base = 8;
			}
		} else {
			base = 10;
		}
	} else if (base == 16 && str[0] == '0') {
		if (str[1] == 'x' || str[1] == 'X') {
			str+=2;
		}
	}
	
	while(*str) {
		if (isupper(*str)) {
			v = *str - 'A'+10;
		} else if (islower(*str)) {
			v = *str - 'a'+10;
		} else if (isdigit(*str)) {
			v = *str - '0';
		} else {
			break;
		}

		if (v >= base) break;

		ret = ret * base + v;
		if (ret < prev_ret) {
			if (sign) {
				return LONG_MIN;
			} else {
				return LONG_MAX;
			}
		}
		prev_ret = ret;
		str++;
	}

	if (endp) *endp = str;

	if (sign) {
		return -ret;
	} else {
		return ret;
	}
}
