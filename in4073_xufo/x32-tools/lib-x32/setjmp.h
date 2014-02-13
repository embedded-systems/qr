/* SETJMP/LONGJMP LIMITS:
			setjmp is a function to store the current processor state, so that later
			on, longjmp can be used to jump back to the point setjmp was called.
			This makes it possible to bypass several returns from deeply nested
			functions, however, using setjmp/longjmp is very dangerous! This is
			caused by the fact that the stack, which contains all intermediate 
			results of a program, is not completely restored, therefore it is very
			easy to "break" the setjmp and longjmp behaviour. The setjmp saves only
			the top of the stack, and assumes this is an address (void*). This is
			done because LCC always stores a result from a function in a (temporary)
			local variable, and therefore, the address of the variable must be on the
			stack just before the functions return value, after the return from 
			setjmp; in general the stack should look like this. The address of this
			temporary value is automatically generated when making a call to setjmp,
			but not when making a call to longjmp, hence the need of saving/restoring
			this value. This limits the use of setjmp in bytecode: always make sure
			the call to setjmp is preceeded by some address generation code, and
			followed by an 'ASGN' instruction (just how it's done by LCC). In C code,
			the use of setjmp is limited by the fact that not the entire stack is
			restored, and setjmp can thus NEVER be a part of a complex equation (as
			also stated in the official C language specifications). In general: use
			setjmp only in a statement containing only the assignment of setjmp to
			a local variable (somvar = setjmp();), and check somevar later on in any
			code construct required by your program. */

/* The structure containing the processor state, note this structure is copied
			from the stack 1 to 1, the order, as well as the sizes of all these 
			variables are thus very important. This structure mus be exacly the same
 			as the X32 saves it's state on a CALL instruction. */
typedef struct {
	void* tos;		/* top of stack */
	void* ra;			/* return address */
	void* el;			/* execution level */
	void* ap;			/* argument pointer */
	void* lp;			/* local pointer */
	void* fp;			/* frame pointer */
} _registers;

/* The jmp_buf structure, this contains the processor state, and the address
			where it should be restored to. */
typedef struct {
	_registers registers;
	void* location;
} jmp_buf;

/* The amount of stack bytes 'skipped' when calling longjmp. This is required
			for applications in which the call to setjmp is placed deeper on the 
			stack than the call to longjmp. setjmp should be placed as high as 
			possible, but will often still be part of at least an assignment (to
			save the return value from setjmp). Since longjmp is called as a function
			returning a void, it could be higher on the stack than setjmp, as in the
			following example:
			i = setjmp(env);
			longjmp(env, j);
			A wrapper around longjmp will skip some stack bytes such that restoring
			the state won't interfear with the running stack of longjmp. In the
			above example, longjmp must be moved 4 bytes deeper onto the stack 
			(caused by the 4 byte pointer to i which is placed on the stack before
			calling setjmp, but not when calling longjmp). This is also the deepest
			stack position setjmp should ever be used in, however, just for safety,
			the default value is 16 */
#define _LONGJMP_STACKBUFF 16

/* the address of the env parameter is required rather then	the actual 
			structure, use #define to get the address */
#define setjmp(env) _setjmp(&env)
/* note that officially, longjmp returns a void, while in	this case, it 
			returns an integer (required to generate the RETI instruction which 
			returns from longjmp to setjmp. However, calling an int function as if 
			it was a void function causes no problems. */
int longjmp(jmp_buf env, int val);

/* functions supported by the library, please don't call any of these directly.
			*/

/* return the framepointer (note, it actually returns the framepointer of the
			calling function, since the framepointer changes with the call to 
			_get_fp() */
void* _get_fp();
/* set the framepointer (again it sets the framepointer of the calling 
			function */
void _set_fp(void*);
/* the 'real' setjmp function */
int _setjmp(jmp_buf *env);
/* the 'real' longjmp function */
int _longjmp(jmp_buf env, int val);
