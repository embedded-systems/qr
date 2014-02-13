#include <string.h>

int strcmp(const char *cs, const char *ct) {
	char *str1 = (char*)cs;
	char *str2 = (char*)ct;

	while(*str1 & *str2) {
		if (*str1 > *str2) {
			return 1;
		} else if (*str1 < *str2) {
			return -1;
		}
		
		str1++;	str2++;
	}
	
	if (*str1 == 0 && *str2 == 0) {
		return 0;
	} else if (*str1 == 0) {
		return -1;
	} else {
		return 1;
	}
}
