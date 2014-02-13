#include <x32.h>
#include <setjmp.h>

void** init_stack(void** stack, void* address, void* parameter) {
	stack[0] = parameter;			/* parameter */
	stack[1] = address;				/* ra (pc) */
	stack[2] = 0;							/* el */
	stack[3] = &stack[0];			/* ap */
	stack[4] = &stack[1];			/* lp */ 
	stack[5] = &stack[1];			/* fp */
	return &stack[6];
}

void context_switch(void** new_fp, void*** old_fp) {
	*old_fp = _get_fp();
	_set_fp(new_fp);
}
