#ifndef _ASSERT
#define _ASSERT

void assert(int);

#endif //_ASSERT

#undef assert
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
int _assert(char *, char *, unsigned);
#define assert(e) ((void)((e)||_assert(#e, __FILE__, __LINE__)))
#endif // NDEBUG
