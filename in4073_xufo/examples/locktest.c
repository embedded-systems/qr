/*
 * This program illustrates the use of the lock() and unlock() functios
 *	supplied by the x32 library. A lock() function will lock a special
 *	variable of type LOCK. The function returns 1 when it locked succesfull,
 *	or 0, when it was allready locked. The lock must be unlocked using
 *	unlock(). (Note, that the lock must also be initialized using unlock()).
 *
 *	The function is completely atomic, and it is therefore guaranteed that
 *	no two threads will ever be able to lock the same lock at the same time.
 *	It is up to the thread what to do when a variable is locked (usually, this
 *	would result in suspending the thread until it gets unlocked).
 *
 *	The program simulates two threads using a hardware timer which triggers an
 *	interrupt. The interrupt prints a sharp sign if the lock is unlocked. The
 *	main program loop prints bursts of dots. During the printing, the lock is
 *	locked. When locks are disabled (uncomment #define USELOCKS), sharps will
 *	appear anywhere between the bursts of dots, while when locks are enabled,
 *	they will only appear at the end of a burst.
 *
 */
#include <x32.h>

// comment this line to test without the use of locks
#define USELOCKS

LOCK the_lock;

void interrupt() ;

#ifndef USELOCKS
	#ifdef lock 
		#undef lock
	#endif
	#ifdef unlock 
		#undef unlock
	#endif
	#define lock(arg) 1
	#define unlock(arg) 0
#endif

int main() {
	int i;
	peripherals[PERIPHERAL_TIMER1_PERIOD] = CLOCKS_PER_MS*5;
	SET_INTERRUPT_VECTOR(INTERRUPT_TIMER1, &interrupt);
	SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER1, 10);
	ENABLE_INTERRUPT(INTERRUPT_TIMER1);
	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);

	unlock(the_lock);

	while(1) {
		if (lock(the_lock)) {
			putchar('\r');
			putchar('\n');
			for (i = 0; i < 50; i++) putchar('.');
			unlock(the_lock);
		}
		/* small wait loop to give the interrupt some time to print */
		for (i = 0; i < 1000; i++);
	}
	return 0;
}

void interrupt() {
	if (lock(the_lock)) {
		putchar('#');
		unlock(the_lock);
	}
}
