#include <stdarg.h>

void _va_init(va_list* ap, void* lastarg, int size) {
	ap->ptr = (void*)((int)lastarg+size);
}

void* _va_next(va_list* ap, int size) {
	void* ret;
	ret = (void*)((int)ap->ptr+4-size);
	ap->ptr = (void*)((int)ap->ptr+4);
	return ret;
}

void _va_end(va_list* ap) {

}

