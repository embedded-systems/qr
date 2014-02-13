#ifndef _STDIO
#define _STDIO

#include <stdarg.h>
#include <stddef.h>

/* just for compatibility, not used */
#define stdin 	(FILE*)1
#define stdout 	(FILE*)2
#define stderr 	(FILE*)3
typedef void FILE;

/* size of buffer used by printf to store formatted numbers in (note;
		this is without padding, '\0' or sign, so theoretically, the number 
		of characters	occupied by the largest number (2^32) represented in 
		the smallest base system (octal) would be enough. */
#define _PRINTF_BUFFER_SIZE 32

/* writes a string to the console */
int puts(char*);
/* write formatted string to stdout */
int printf(const char*, ...);
int vprintf(const char*, va_list);
/* write formatted string to buffer */
int sprintf(char*, const char*, ...);
int vsprintf(char*, const char*, va_list);

/* for internal use, one engine for printf, sprintf and fprinf */
int _xprintf(int (*)(int, void*), void*, const char*, va_list*);

/* ignore stream identifier */
#define fputs(msg, stream) puts(msg)
#define fputc(msg, stream) putchar(msg)

//is something like this possible?:
//#define fprintf(stream, fmt, ...) printf(fmt, ...)

/* this function is NOT part of the standard library for C, however, it was
 *	present in the first versions of the X32 C library, when there was no
 *	printf available. It is now only here for compatibility reasons (note,
 *	it is just a macro pointing to printf)
 */
//int print_int(int);
//#define print_int(arg) printf("%d", arg)

#endif // _STDIO
