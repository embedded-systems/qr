/* see io.h for general information */
#include "io.h"

#ifdef _WIN32
	/* windows code */
	#include <windows.h>
	
	HANDLE hIn, hOut;		/* windows handle to stdin and stdout */
	DWORD flags;				/* stdin flags */
	
	/*
	 * initialize io functions 
	 */
	int init_io() {
		/* get handle to stdin */
		hIn = GetStdHandle(STD_INPUT_HANDLE);
		/* get handle to stdout */
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		
		/* test */
		if (hIn == 0 || hOut == 0) return 0;
		
		/* save existing flags */
		GetConsoleMode(hIn, &flags);
		
		/* unflag all modes */
		SetConsoleMode(hIn, 0x00);
		
		/* ok */
		return 1;
	}
	
	/*
	 *	terminate io functions
	 */
	void term_io() {
		/* reset old flags */
		SetConsoleMode(hIn, flags);
	}
	
	/*
	 *	read one character (blocking)
	 */
	int read_char() {
		TCHAR c;			/* char */
		DWORD count;	/* number of chars */
		
		/* try to read one byte from stdin */
		ReadConsole(hIn, &c, 1, &count, NULL);
		
		/* check number of bytes succesfully read */
		if (count == 1) {
			/* return byte */
			return c;
		} else {
			/* return -1 */
			return -1;
		}
	}
	
	/*
	 *	read one character (non blocking)
	 */
	int read_char_non_blocking() {
		DWORD event_count, count;
		INPUT_RECORD rec;
		
		/* get the number of pending events (keypresses, mouseclicks) */
		if (GetNumberOfConsoleInputEvents(hIn, &event_count)) {
			/* check them all */
			while(event_count-- > 0) {
				/* get the event data */
				if (ReadConsoleInput(hIn, &rec, 1, &count) && count > 0) {
					/* must be a keydown event */
					if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
						/* return the key pressed */
						return rec.Event.KeyEvent.uChar.AsciiChar;
					}
				} else {
					/* read error */
					return -1;
				}
			}
			/* no available key events */
			return -1;
		} else { 
			/* API call error */
			return -1;
		}
		
	}
	
	/*
	 * write a character
	 */
	void write_char(char c) {
		/* use standard c call */
		putc(c, stdout);
	}
#else 
	/* linux code */
	#include <stdio.h>
	#include <termios.h>
	#include <unistd.h>
	#include <poll.h>

	struct termios savetty;

	/*
	 * initialize io functions 
	 */
	struct pollfd fds[1];

	int init_io() {
		struct termios tty;

		/* backup console */		
		tcgetattr(0, &savetty);
		tcgetattr(0, &tty);
		/* set console flags */
		tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
		//tty.c_lflag &= ~ICANON;
		tty.c_cc[VTIME] = 0;
		tty.c_cc[VMIN] = 0;
		tcsetattr(0, TCSADRAIN, &tty);
		
		fds[0].fd = 0;
		fds[0].events = POLLIN;

		return 1;
	}
	
	/*
	 *	terminate io functions
	 */
	void term_io() {
		/* reset console */
		tcsetattr(0, TCSADRAIN, &savetty);
	}

	/*
	 * write a character
	 */
	void write_char(char c) {
		/* use standard c call */
		putc(c, stdout);
		fflush(stdout);
	}

	/*
	 *	read one character (non blocking)
	 */
	int read_char_non_blocking() {
		/* return -1 if no char available else char */
		static char     foo;

		/* allows the debugger to exit the interpreter */
		if ((fds[0].revents & POLLHUP) != 0) {
				printf("Exitting...\n");
				exit(2);
		}
		/* note: destructive read */

		if( poll(fds, 1, 0) > 0) {
		    read(0, &foo, 1);
                    return foo;
                }
		
		return -1;
	}

	/*
	 *	read one character (blocking)
	 */
	int read_char() {
		char c;
		/* keep calling non blocking read till a character is read */
		while ((c = read_char_non_blocking()) == -1);
		return c;
	}

#endif
