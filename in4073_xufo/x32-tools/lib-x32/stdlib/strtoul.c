#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

unsigned long strtoul(const char* s, char** endp, int base) {
	char *str = (char*)s;
	int v;
	unsigned long ret = 0;
	unsigned long prev_ret = 0;

	while (*str == ' ' || *str == '\t') str++;

	if (*str == '-') {
		return ULONG_MAX;
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
			return ULONG_MAX;
		}
		prev_ret = ret;
		str++;
	}

	if (endp) *endp = str;

	return ret;
}
