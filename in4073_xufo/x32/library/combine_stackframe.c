#include <x32.h>
#include <setjmp.h>

void combine_stackframe() {
	/* get the current frame pointer */
	void* current_fp = (void*)_get_fp();
	/* get (a pointer to) the frame pointer of the calling function */
	void** handler_fp = (void**)((char*)current_fp-sizeof(void*));
	/* get (a pointer to) the frame pointer of the calling calling function */
	void** raiser_fp = (void**)((char*)*handler_fp-sizeof(void*));
	/* get (a pointer to) the argument pointer of the calling function */
	void** handler_ap = (void**)((char*)current_fp-3*sizeof(void*));
	/* get (a pointer to) the argument pointer of the calling calling function */
	void** raiser_ap = (void**)((char*)*handler_fp-3*sizeof(void*));
	
	/* make the calling function return as if it was the 
		calling calling function */
	*handler_fp = *raiser_fp;
	/* make the calling function use the arguments passed to the
		calling calling function */
	*handler_ap = *raiser_ap;
}
