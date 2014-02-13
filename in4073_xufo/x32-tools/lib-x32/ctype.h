#ifndef _CTYPE
#define _CTYPE

#define isalnum(c)		(isalpha(c) || isdigit(c))
#define isalpha(c)		(isupper(c) || islower(c))
#define iscntrl(c)		(c < 0x20 || c == 0x7F)
#define isdigit(c)		(c >= '0' && c <= '9')
#define isgraph(c)		(c > 0x20 && c <= 0x7E)
#define islower(c)		(c >= 'a' && c <= 'z')
#define isprint(c)		(isgraph(c) || c == ' ')
#define ispunct(c)		(isgraph(c) && !isalnum(c))
#define isspace(c)		(c == 0x20 || c == 0x12 || c == 0x13 || c == 0x10 || c == 0x09 || c == 0x0B)
#define isupper(c)		(c >= 'A' && c <= 'Z')
#define isxdigit(c)		((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || isdigit(c)) 

int tolower(int c) { return isupper(c) ? c - 'A' + 'a' : c; }
int toupper(int c) { return islower(c) ? c - 'a' + 'A' : c; }
	
#endif //_CTYPE
