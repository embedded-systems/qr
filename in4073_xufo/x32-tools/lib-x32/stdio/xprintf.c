#include <stdio.h>
#include <ctype.h>

/*
 * xprintf, engine for printf, sprintf, fprintf etc.
 *	Implemented: '0', '-'
 *		Flags: '0', '-'
 *		Numbers specifieng the field width
 *		Types: d, i, o, x, X, u, c, s, p, n, %
 *	Not implemented: 
 *  	Flags: '+', ' ', '#'
 *		Precision period + number
 *		Length modifiers
 *		Types: e, E, g, G
 *
 * TODO: Flags '+', ' ' and '#', all floating point stuff will not be 
 *		implemented
 */

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
static int _uint2str(char*, int, unsigned, int);

/* write a buffer with left & right padding */
static int _write_padded_buffer(int, int, int, int, int(int, void*), void*, 
	char*, int, int);


/*
 * xprintf, send a formatted string to the charhandler function, this function
 *	can then send the characters to stdout, a file, or a string, such that only
 *	one 'xprintf' engine is required for the printf, fprintf and sprintf
 *	functions.
 */
int _xprintf(int charhandler(int, void*), void *param, const char* fmt, va_list *ap) {
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

	int force_sign;				/* force sign (+) */
	int add_prefix;				/* add prefix for %o, %x or %X */

	/* the buffer for _uint2str */
	char buffer[_PRINTF_BUFFER_SIZE];
	
	/* character loop */
	i = chars = 0;
	while(fmt[i] != 0) {
		if (fmt[i] == '%') {
			/* start reading a format string, reset all variables */
			pad = ' ';			/* pad with spaces */
			alignment = 0;	/* right alignment */
			count = -1;			/* no count specified */
			is_fmt = 1;			/* reading format string */
			force_sign = 0;	/* don't add '+' sign */
			add_prefix = 0; /* don't add prefix */

			/* handle the entire format string */
			while(is_fmt && fmt[++i] != '\0') {
				if (fmt[i] == '%') {
					/* percentage character */
					chars += charhandler('%', param);
					/* end of format */
					is_fmt = 0;
				} else if (count == -1 && fmt[i] == '-') {
					/* a '-' sets the alignment to left, must be first character */
					alignment = 1;
				} else if (count == -1 && fmt[i] == '0') {
					/* a leading zero sets the padding character to '0' */
					pad = '0';
				} else if (count == -1 && fmt[i] == '#') {
					add_prefix = 1;
				} else if (count == -1 && fmt[i] == '+') {
					force_sign = 1;
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
							value = va_arg(*ap, int);

							/* handle negative numbers differently */
							if (value < 0) {
								/* convert the (negative) parameter into a (positive) string */
								buffsize =  _uint2str(buffer, _PRINTF_BUFFER_SIZE, (unsigned int)-value, 10);

								/* write buffer and padding */
								chars += _write_padded_buffer(alignment, buffsize, count, pad, 
									charhandler, param, buffer, 1, 0);

							} else {
								/* convert the parameter into a string */
								buffsize =  _uint2str(buffer, _PRINTF_BUFFER_SIZE, (unsigned int)value, 10);

								/* write buffer and padding */
								chars += _write_padded_buffer(alignment, buffsize, count, pad, 
									charhandler, param, buffer, 0, 0);
							}
							
							/* end of format string */
							is_fmt = 0;
							break;
						case 'u':
							/* convert the parameter into a string */
							uvalue = va_arg(*ap, unsigned int);
							buffsize =  _uint2str(buffer, _PRINTF_BUFFER_SIZE, uvalue, 10);

							/* write buffer and padding */
							chars += _write_padded_buffer(alignment, buffsize, count, pad, 
								charhandler, param, buffer, 0, 0);

							/* end of format string */
							is_fmt = 0;
							break;
						case 'o':
							/* convert the parameter into a string */
							uvalue = va_arg(*ap, unsigned int);
							buffsize =  _uint2str(buffer, _PRINTF_BUFFER_SIZE, uvalue, 8);

							/* write buffer and padding */
							chars += _write_padded_buffer(alignment, buffsize, count, pad, 
								charhandler, param, buffer, 0, 0);

							/* end of format string */
							is_fmt = 0;
							break;
						case 'x':
							/* convert the parameter into a string */
							uvalue = va_arg(*ap, unsigned int);
							buffsize =  _uint2str(buffer, _PRINTF_BUFFER_SIZE, uvalue, 16);

							/* write buffer and padding */
							chars += _write_padded_buffer(alignment, buffsize, count, pad, 
								charhandler, param, buffer, 0, 1);

							/* end of format string */
							is_fmt = 0;
							break;
						case 'X': case 'p':
							/* pointers are written as upper case hexadecimal value */
							/* convert the parameter into a string */
							uvalue = va_arg(*ap, unsigned int);
							buffsize =  _uint2str(buffer, _PRINTF_BUFFER_SIZE, uvalue, 16);

							/* write buffer and padding */
							chars += _write_padded_buffer(alignment, buffsize, count, pad, 
								charhandler, param, buffer, 0, 0);

							/* end of format string */
							is_fmt = 0;
							break;
						case 's':
							/* get char* */
							svalue = va_arg(*ap, char*);
		
							/* write character */
							if (svalue == 0) svalue = "(null)";
							while(*svalue) chars += charhandler(*svalue++, param); 						

							/* end of format string */
							is_fmt = 0;
							break;
						case 'c':
							/* get char */
							cvalue = va_arg(*ap, char);
		
							/* write character */
							chars += charhandler(cvalue, param); 						

							/* end of format string */
							is_fmt = 0;
							break;
						case 'n':
							/* not yet implemented */
							ivalue = va_arg(*ap, int*);
							
							/* store chars in *ivalue */
							*ivalue = chars;

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
			chars += charhandler(fmt[i], param);
		}
		
		/* next character */
		i++;
	}

	/* return number of characters written */
	return chars;
}

/* 
 * convert an unsigned number into a string, given its base. store
 *	the result (in reverse order) in buffer, make sure it'll never
 *	generate more characters than buffsize.
 */
static int _uint2str(char* buffer, int buffsize, unsigned value, int base) {
	int i;

	/* handle 0 */
	if (value == 0) {
		buffer[0] = '0';
		return 1;
	}

	/* convert in reverse order */
	for (i = 0; (i < buffsize) && value; i++) {
		buffer[i] = _symbols[value % base];
		value = value / base;
	}

	/* return number of characters in buffer */
	return i;
}

/* write a buffer with left & right padding */
static int _write_padded_buffer(int alignment, int buffsize, int count, 
	int pad, int charhandler(int, void*), void *param, char* buffer, 
	int negative, int lcase) {

	int i;
	int chars = 0;

	if (negative) {
		/* write negative sign (in case of '0' padding) */
		if (pad != ' ') chars += charhandler('-', param);
		/* write left padding */
		if (alignment == 0) for (i = buffsize; i < count-1; i++) 
			chars += charhandler(pad, param);
		/* write negative sign (in case of ' ' padding) */
		if (pad == ' ') chars += charhandler('-', param);
		/* write buffer (in reverse order) */
		for (i = buffsize-1; i >= 0; i--) 
			chars += charhandler(buffer[i], param);
		/* write right padding */
		if (alignment == 1) for (i = buffsize; i < count-1; i++) 
			chars += charhandler(pad, param);
	} else {
		/* write left padding */
		if (alignment == 0) for (i = buffsize; i < count; i++) 
			chars += charhandler(pad, param);
		/* write buffer (in reverse order) */
		if (lcase) {
			for (i = buffsize-1; i >= 0; i--) 
				chars += charhandler(tolower(buffer[i]), param);
		} else {
			for (i = buffsize-1; i >= 0; i--) 
				chars += charhandler(buffer[i], param);
		}
		/* write right padding */
		if (alignment == 1) for (i = buffsize; i < count; i++) 
			chars += charhandler(pad, param);
	}

	return chars;
}
