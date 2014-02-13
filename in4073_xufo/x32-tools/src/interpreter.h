/*
 *	interpreter: contains startup code for the interpreter, 
 *		the actual interpreter code is defined in 
 *		interpreter_engine.c
 *
 *	Author: Sijmen Woutersen
 */

#include "interpreter_engine.h"
#include "bool.h"
#include <stdio.h>

/* default amount of memory (1MB) */
#define DEF_MEMORY_SIZE 1048576
/* default version id */
#define DEF_VID 0x0020

/* print usage to screen */
void printUsage();
/* convert error code to error message */
char* errmsg(int errcode);
/* convert hex string to integer */
int hex2bin(char*);
