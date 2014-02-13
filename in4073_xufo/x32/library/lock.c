#include <x32.h>

int _lock(LOCK* var) {
	/* switch to ultra high execution level.
			Note: it is not required to save the old el, since it is automatically
			restored on function return */
	set_execution_level(THREAD_SAFE_EXECUTION_LEVEL);
	
	/* it is impossible for the rest of this function to get interrupted */

	if (*var) {
		/* failed, var is allready locked */
		return 0;
	} else {
		*var = 1;
		return 1;
	} 
}

void _unlock(LOCK* var) {
	*var = 0;
}
