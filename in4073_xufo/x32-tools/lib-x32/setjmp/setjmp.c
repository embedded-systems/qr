#include <setjmp.h>

int _setjmp(jmp_buf *env) {
	_registers *current_registers;
	
	/* set the location of the register dump, created by the call
			to setjmp */
	env->location = (void*)((int)_get_fp() - sizeof(_registers));
	/* set the pointer to the current_register structure to this
			value, current_registers is now a structure containing
			all processor registers (before the call to setjmp) */
	current_registers = (_registers *)(env->location);
	/* copy the registers structure */
	env->registers = *current_registers;
	/* return zero, a standard return from setjmp */
	return 0;
}
