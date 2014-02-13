#include "shell.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <x32.h>

void insert_char(TERMINAL* buffer, unsigned char chr);
void backspace(TERMINAL* buffer);
void move_cursor_right(TERMINAL* buffer);
void move_cursor_left(TERMINAL* buffer);
int parse_line(char* line, char** args, int max);

void open_terminal(TERMINAL *term) {
	term->line_length = 0;
	term->arg_count = 0;
	term->cursor_position = 0;
}

void close_terminal(TERMINAL *term) {
}

void get_command(TERMINAL *term) {
	unsigned char c = 0;

	term->line_length = 0;
	term->arg_count = 0;
	term->cursor_position = 0;

	while (c != '\n' && c != '\r') {
		switch(c = getchar()) {
			case 0x1B: // ESCAPE:
				switch (c = getchar()) {
					case 0x5B: // ARROW
						switch (c = getchar()) {
							case 0x41: // UP
								break;
							case 0x42: // DOWN
								break;
							case 0x43: // RIGHT
								move_cursor_right(term);
								break;
							case 0x44: // LEFT
								move_cursor_left(term);
								break;
						}
				}
				break;
			case '\b': case 0x7F:
				backspace(term);
				break;
			case '\r': case '\n':
				break;
			default:
				insert_char(term, c);
				break;
		}
	}
	
	term->line[term->line_length] = '\0';
	putchar('\r');
	putchar('\n');

	strcpy(term->arg_buffer, term->line);
	term->arg_count = parse_line(term->arg_buffer, term->args, MAX_ARGS);
} 

void insert_char(TERMINAL *term, unsigned char chr) {
	int i;

	if (term->line_length < MAX_LINE_LENGTH-1) {
		for (i = term->line_length; i > term->cursor_position; i--) {
			term->line[i] = term->line[i-1];
		}
		term->line[term->cursor_position] = chr;
		
		term->line_length++;
		if (term->cursor_position < term->line_length) term->cursor_position++;
	
		for (i = term->cursor_position-1; i < term->line_length; i++) {
			putchar(term->line[i]);
		}
		for (i = term->cursor_position; i < term->line_length; i++) {
			putchar('\b');
		}
	}
}

void backspace(TERMINAL *term) {
	int i;

	if (term->cursor_position > 0) {	
		putchar('\b');
		for (i = term->cursor_position; i < term->line_length; i++) {
			putchar(term->line[i]);
			term->line[i-1] = term->line[i];
		}
		putchar(' ');

		term->cursor_position--;

		for (i = term->cursor_position; i < term->line_length; i++) {
			putchar('\b');
		}

		term->line_length--;
	}
}

void move_cursor_right(TERMINAL *term) {
	if (term->cursor_position < term->line_length) {
		putchar(term->line[term->cursor_position]);
		term->cursor_position++;
	}
}

void move_cursor_left(TERMINAL *term) {
	if (term->cursor_position > 0) {
		term->cursor_position--;
		putchar('\b');
	}
}


int parse_line(char* line, char** args, int max) {
	int chr_in_line = 0;
	int chr_in_arg = 0;
	int arg = 0;
	int inquote = 0;

	args[0] = line;

	while(line[chr_in_line]) {
		if (inquote) {
			switch (line[chr_in_line]) {
				case '\"':
					line[chr_in_line] = '\0';
					if (chr_in_arg) {
						chr_in_arg = 0;
						arg++;
					}
					args[arg] = &line[chr_in_line+1];
					inquote = 0;
					break;
				default:
					chr_in_arg++;
					break;
			}
		} else {
			switch (line[chr_in_line]) {
				case '\"':
					inquote = 1;
				case '\t': case ' ': 
					line[chr_in_line] = '\0';
					if (chr_in_arg) {
						chr_in_arg = 0;
						arg++;
					}
					args[arg] = &line[chr_in_line+1];
					break;
				default:
					chr_in_arg++;
					break;
			}
		}
		chr_in_line++;
	}

	if (chr_in_arg) arg++;
	return arg;
}


















/* numerical symbols, can be indexed as an array, works for binary, octal,
 *	decimal and hexadecimal
 */
char* _symbols = "0123456789ABCDEF";

/* convert a (unsigned) number to a string, the string is placed in the buffer
 *	(1st parameter) which will never be larger than the second parameter. The
 *	numerical value is given through the 3rd parameter, the fourth parameter
 *	holds the base (should be 2, 8, 10 or 16). The buffer is filled in reverse
 *	order!
 */
int _uint2str(char*, int, unsigned, int);

int stripped_printf(const char* fmt, ...) {
	int i, j;							/* loop counters */
	int value;						/* signed parameter */
	unsigned int uvalue;	/* unsigned parameter */
	char* svalue;					/* string parameter */
	char cvalue;					/* char parameter */
	int* ivalue;					/* int* parameter */
	int buffsize;					/* size of the buffer returned by _uint2str */

	char pad;							/* number padding symbol (' ' or '0') */
	int alignment;				/* number alignment (0 = right, 1 = left) */
	int is_fmt;						/* indicates the current characters are a formatting 
														string */
	int count;						/* requested number length (in characters) */

	int chars;						/* total number of characters written */

	va_list ap;

	/* the buffer for _uint2str */
	char buffer[PRINTF_BUFFER_SIZE];
	
	/* start reading arguments */
	va_start(ap, fmt);

	/* character loop */
	i = chars = 0;
	while(fmt[i] != 0) {
		if (fmt[i] == '%') {
			/* start reading a format string, reset all variables */
			pad = ' ';			/* pad with spaces */
			alignment = 0;	/* right alignment */
			count = -1;			/* no count specified */
			is_fmt = 1;			/* reading format string */

			/* handle the entire format string */
			while(is_fmt && fmt[++i] != '\0') {
				if (fmt[i] == '%') {
					/* percentage character */
					chars++;
					putchar('%');
					/* end of format */
					is_fmt = 0;
				} else if (count == -1 && fmt[i] == '-') {
					/* a '-' sets the alignment to left, must be first character */
					alignment = 1;
				} else if (count == -1 && fmt[i] == '0') {
					/* a leading zero sets the padding character to '0' */
					pad = '0';
				} else if (isdigit(fmt[i])) {
					/* any following digits make up for the (minimal) length (in 
							characters)*/
					if (count == -1) {
						/* first number */
						count = fmt[i] - '0';
					} else {
						/* 2nd ... numbers */
						count = count * 10 + (fmt[i] - '0');
					}
				} else {
					/* none of the above, should be an argument descriptor */
					switch (fmt[i]) {
						case 'd': case 'i':
							/* decimal (signed integer) */
							value = va_arg(ap, int);

							/* handle negative numbers differently */
							if (value < 0) {
								/* convert the (negative) parameter into a (positive) string */
								buffsize =  _uint2str(buffer, PRINTF_BUFFER_SIZE, (unsigned int)-value, 10);

								/* write negative sign (in case of '0' padding) */
								if (pad != ' ') {
									putchar('-');
									chars ++;
								}
								/* write left padding */
								if (alignment == 0) for (j = buffsize; j < count-1; j++) {
									putchar(pad);
									chars++;
								}
								/* write negative sign (in case of ' ' padding) */
								if (pad == ' ') {
									chars += putchar('-');
									chars++;
								}
								/* write buffer (in reverse order) */
								for (j = buffsize-1; j >= 0; j--) {
									putchar(buffer[j]);
									chars++;
								}
								/* write right padding */
								if (alignment == 1) for (j = buffsize; j < count-1; j++) {
									putchar(pad);
									chars++;
								}
							} else {
								/* convert the parameter into a string */
								buffsize =  _uint2str(buffer, PRINTF_BUFFER_SIZE, (unsigned int)value, 10);

								/* write left padding */
								if (alignment == 0) for (j = buffsize; j < count; j++) {
									putchar(pad);
									chars++;
								}
								/* write buffer (in reverse order) */
								for (j = buffsize-1; j >= 0; j--) {
									putchar(buffer[j]);
									chars++;
								}
								/* write right padding */
								if (alignment == 1) for (j = buffsize; j < count; j++) {
									putchar(pad);
									chars++;
								}
							}
							
							/* end of format string */
							is_fmt = 0;
							break;
						case 'u': case 'x': case 'X': case 'p':
							/* pointers are written as upper case hexadecimal value */
							/* convert the parameter into a string */
							uvalue = va_arg(ap, unsigned int);
							buffsize =  _uint2str(buffer, PRINTF_BUFFER_SIZE, uvalue, 16);
							/* write left padding */
							if (alignment == 0) for (j = buffsize; j < count; j++) {
								putchar(pad);
								chars++;
							}
							/* write buffer (in reverse order) */
							for (j = buffsize-1; j >= 0; j--) {
								putchar(buffer[j]);
								chars++;
							}
							/* write right padding */
							if (alignment == 1) for (j = buffsize; j < count; j++) {
								chars ++;
								putchar(pad);
							}

							/* end of format string */
							is_fmt = 0;
							break;
						case 's':
							/* get char* */
							svalue = va_arg(ap, char*);
		
							/* write character */
							while(*svalue) {
								chars ++;
								putchar(*svalue++); 						
							}

							/* end of format string */
							is_fmt = 0;
							break;
						case 'c':
							/* get char */
							cvalue = va_arg(ap, char);
		
							/* write character */
							putchar(cvalue);
							chars ++;

							/* end of format string */
							is_fmt = 0;
							break;
						default:
							/* error! invalid format, ignore all format chars */
							/* end of format string */
							is_fmt = 0;
							break;
					}
				}
				
			}
		} else {
			/* standard character, pass on to character handler */
			putchar(fmt[i]);
			chars ++;
		}
		
		/* next character */
		i++;
	}

	/* stop reading parameters */
	va_end(ap);

	/* return number of characters written */
	return chars;
}

/* 
 * convert an unsigned number into a string, given its base. store
 *	the result (in reverse order) in buffer, make sure it'll never
 *	generate more characters than buffsize.
 */
int _uint2str(char* buffer, int buffsize, unsigned value, int base) {
	int i;

	/* handle 0 */
	if (value == 0) {
		buffer[0] = '0';
		return 1;
	}

	/* convert in reverse order */
	for (i = 0; i < buffsize && value; i++) {
		buffer[i] = _symbols[value % base];
		value = value / base;
	}
	/* return number of characters in buffer */
	return i;
}
