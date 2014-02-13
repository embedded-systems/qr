#ifndef _STRING
#define _STRING

#include <stddef.h>


void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
char *strcat(char *, const char *);
char *strchr(const char *, int);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, int);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, int);
size_t strcspn(const char *, const char *);
char *strerror(int);
size_t strlen(const char *);
size_t strspn(const char *, const char *);
char *strtok(char*, const char*);

char *strlwr(char*);
char *strupr(char*);

// no locale settings
#define strcoll strcmp

#endif //_STRING
