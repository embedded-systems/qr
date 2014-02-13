/*
 * Assembler: Contains some OS specific functions
 *		for writing and reading data to and from the console
 *		currently supports windows and linux
 *
 *	Author: Sijmen Woutersen
 */
#include <stdio.h>
#include <stdlib.h>

/* return a random integer */
unsigned rnd();
/* read a character */
int read_char();
/* read a character, don't block */
int read_char_non_blocking();
/* write a character */
void write_char(char);
/* initialize */
int init_io();
/* terminate */
void term_io();
