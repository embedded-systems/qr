/*
 * library: Contains only one function for loading library files
 *		these libraries must be created with the linker 
 *		(parameter -createlib)
 *
 *		A library is simple a collection of object files; if any
 *		function form a library is imported, the entire object file
 *		is imported. Library functions can import other library
 *		functions, they can be in different objects.
 *
 *		Example:
 *				library stdio
 *					object 1
 *						puts()
 *						putchar()
 *						getchar()
 *					object 2
 *						printf() (uses putchar)
 *
 *	if a c program uses printf, it will automatically get puts(),
 *	putchar() and getchar(), however if it uses puts() it won't get
 *	printf().
 *
 *	For efficient executables, libraries should contain a lot of small
 *	objects (less functions per object = better). Unefficient libraries
 *	are not a problem (linking is done only ones, and the libraries stay
 *	at a "large" computer.
 *
 *	Author: Sijmen Woutersen
 */
 
/*
 *	Library file format:
 *		#bytes	description
 *		04			magic marker: 'S' 'L' 'I' 'B'
 *		04			nr of object files
 *		xx			object files
 *
 *		object file:
 *		04			size
 *		xx			data (complete object file, see assembler.h)
 */

#ifndef LIBRARY_H
#define LIBRARY_H 

#include "bool.h"
#include <stdio.h>
#include <stdlib.h>
#include "memstream.h"
#include "hashtable.h"
#include "linker.h"

/* load a library file/directory */
BOOL load_libdir(char*, HashTable*);
BOOL load_libfile(char*, HashTable*);
/* load the loader */
Assembly* load_loader(HashTable*, char*);

#endif
