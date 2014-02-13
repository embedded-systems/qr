#include <string.h>

int strncmp(const char *cs, const char *ct, int n) {
	char *str1 = (char*)cs;
	char *str2 = (char*)ct;

	while((*str1 & *str2) && (n > 0)) {
		if (*str1 > *str2) {
			return 1;
		} else if (*str1 < *str2) {
			return -1;
		}
		
		str1++;	str2++; n--;
	}
	
	if (*str1 == 0 && *str2 == 0 || n == 0) {
		return 0;
	} else if (*str1 == 0) {
		return -1;
	} else {
		return 1;
	}
}
