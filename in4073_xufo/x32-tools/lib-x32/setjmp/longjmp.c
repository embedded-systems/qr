#include <setjmp.h>

/* this function is simply a wrapper for _longjmp, the real longjmp function. 
			The wrapper contains some dummy data which is placed on the stack. 
			This dummy data forces _longjmp down the stack, just in case setjmp was 
			placed deeper on the stack than _longjmp (which will under normal 
			circumstances not be the case, but does happen in some examples). This
			is done so _longjmp won't overwrite it's own stack. The amount of stack
			taken by longjmp is defined in setjmp.h by the _LONGJMP_STACKBUFF macro.
			Note that if setjmp is used as part of a complex equation, _longjmp may
			still get in trouble. However, setjmp should NEVER be used in complex
			equations, since this could invalidate the stack at all times. */		
int longjmp(jmp_buf env, int val) {
	char dummy[_LONGJMP_STACKBUFF];
	return _longjmp(env, val);
}

/* the actual longjmp function. This function restores all pointers to the
			values stored in jmp_buf. Several things might go wrong in this function,
			and there is no protection whatsoever. See setjmp.h for more information
			about the limits of setjmp/longjmp */
/* longjmp works by modifieng the stack, such that the next return statement
			executed will be executed just like the return from setjmp. All registers
			plus the top of the stack are restored to the point when setjmp was 
			called. */
int _longjmp(jmp_buf env, int val) {
	_registers *new_registers;
	
	new_registers = (_registers *)env.location;
	/* copy the register contents to the stack */
	*new_registers = env.registers;
	/* now set fp to use this register block on the return */
	_set_fp((void*)((int)env.location + sizeof(_registers)));
	/* the return will be executed as if it was a return from 
			setjmp */
	return val;
}
