#include <stdlib.h>
#include <stdio.h>

/* only initialize first */
void *exit_functions[MAX_EXIT_FUNCTIONS] = {0};

void halt(int);

void exit(int status) {
	void (*fcn)(int);
	int i = 0;
	
 	/* call at exit functions in reverse order */
	while (exit_functions[i++]);
	i--;
	while (i) {
		/* line generates warning, it's ok */
		/* note: the function is converted from void(void) to void(int),
		 *   and the status parameter is passed to the function. Registering void(void)
		 *   still works, but so do void(int) functions (however, they must be converted
		 *   to void(void) functions to get rid of compiler errors/warnings). Since all
		 *   function arguments are stored in the callers stackframe, this won't affect
		 *    the callee's at all.
		 */
		fcn = exit_functions[--i];
		fcn(status);
	}

	_return_from_main(status);
}

int atexit(void (*fcn)(void)) {
	int i = 0;
	
	while(i < MAX_EXIT_FUNCTIONS) {
		if (exit_functions[i] == 0) {
			/* line generates warning, it's ok */
			exit_functions[i] = fcn;
			/* terminate next */
			if (i < MAX_EXIT_FUNCTIONS-1) exit_functions[i+1] = 0;
			return 0;
		}
		i++;
	}
	return -1;
}
