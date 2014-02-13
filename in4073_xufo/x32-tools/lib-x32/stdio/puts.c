int puts(char* s) {
	while(*s) { 
		putchar(*s);
		s++;
	}
	putchar('\r');
	putchar('\n');
	return 1;
}
