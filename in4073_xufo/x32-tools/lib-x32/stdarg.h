#ifndef STDARG_H
#define STDARG_H

typedef struct {
	void* ptr;
} va_list;

#define va_start(ap, lastarg) _va_init(&ap, (void*)&lastarg, sizeof(lastarg));
#define va_arg(ap, type) (*(type*)_va_next(&ap, sizeof(type)))
#define va_end(ap) _va_end(&ap)

void _va_init(va_list* ap, void* lastarg, int size);
void* _va_next(va_list* ap, int);
void _va_end(va_list* ap);

#endif
