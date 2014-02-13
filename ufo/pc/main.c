
#include <stdio.h>

int main()
{

	char c;
	while ((c = getc(stdin)) > 0) {
		printf("%d", c);
	}

	return 0;
}
