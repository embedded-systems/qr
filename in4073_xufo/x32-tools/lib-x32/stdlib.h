#include <stddef.h>

#ifndef _STDLIB
#define _STDLIB

#define EXIT_SUCCESS 	0
#define EXIT_FAILURE 	1

/* maximum random number result */
#define MAX_RAND 0x7FFFFFFF

/* maximum numer of "at exit" functions */
#define MAX_EXIT_FUNCTIONS		0x10

#define abort() exit(EXIT_FAILURE)

int atoi(const char*);
long atol(const char*);
long strtol(const char*, char**, int);

void exit(int);
int atexit(void (*)(void));
int rand();
void srand(unsigned int);

void* malloc(size_t);
void *calloc(size_t, size_t);
void free(void*);

int abs(int);
long labs(long);

// not supported, but still return valid values
#define system(s) NULL
#define getenv(name) NULL


typedef struct {
	long quot;
	long rem;
} ldiv_t;

ldiv_t ldiv(long, long);

typedef struct {
	int quot;
	int rem;
} div_t;

div_t div(int, int);

/* for internal use only; used by exit() */
void _return_from_main(int);

#endif // _STDLIB
