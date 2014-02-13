/*
 * Assembler: Compiles into a modified LCC bytecode assembler.
 *		The LCC bytecode must first be converted into modified
 *		bytecode, which is done by the converter program (see
 *		converter.c).
 *
 *		All debugging information contained in the bytecode files
 *		is lost within the assembler; modify assembler.c to keep
 *		the debugging information.
 *
 *		No label resolving is done in the assembler. All label 
 *		information is stored in a table in the object file, the
 *		linker should do all the label resolving.
 *
 *	Author: Sijmen Woutersen
 */
 
/*
 *	Object file format:
 *		#bytes	description
 *		04			magic marker: 'S' 'O' 'B' 'J'
 *		04			number of labels in object file (int)
 *		??			labels (no data between labels)
 *		??			code segment (instructions)
 *		??			lit segment (constants)
 *		??			data segment (initialized variables)
 *		??			bss segment (uninitialized variables)*
 *		
 *		label:
 *		01			length of label name
 *		xx			label name
 *		01			flags (lower 4 bits)
 *								0x01:		imported (label location is in another file)
 *								0x02:		exported (label is public)
 *            debug info (higher 4 bits)
 *                0x10: line type
 *                0x20: file type
 *                0x30: function type
 *                0x40: global type
 *                0x50: local type
 *                0x60: param type
 *                0x70: field type
 *                0x80: typedef type
 *		01			segment (see instructions.h for values)
 *		04			id (unique) (int)
 *		04			location (int)
 *
 *		segment:
 *		04			size of segment (int)
 *		xx			segment data
 *		xx			segment label mask
 *								the label mask is 1/8 the size of the segment, each 1 (bit)
 *								in the mask represents a label address byte in the segment
 *								data. (Since a label address is always more than one byte
 *								long, bursts of ones will appear in the mask.
 *								every label address in the segment will contain a label id
 *								before it is resolved
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "textparser.h"
#include "bool.h"
#include "memstream.h"
#include "hashtable.h"

#define MAX_LINE 1024		/* maximum number of characters per line */
#define MAX_ARGS 128		/* maximum number of arguments (spaces in line) */

/* some memory space is allocated for each segment at start
		(may grow) */
#define DEFCODESIZE 0x1000
#define DEFDATASIZE 0x1000
#define DEFLITSIZE 0x1000
#define DEFBSSSIZE 0x1000

/* instruction parameter type */ 
#define PARAM_NONE 0			/* no parameter (eg ADD*) */
#define PARAM_CONST 1			/* constant parameter (eg CNST*) */
#define PARAM_POINTER 2		/* pointer parameter (eg ADDRLP*) */
#define PARAM_INT 3				/* number parameter (eg ARGSTACK) */

/* label debug types */
#define LABEL_DEBUG_NONE 0			/* normal label */
#define LABEL_DEBUG_LINE 1			/* line number */
#define LABEL_DEBUG_FILE 2			/* filename */
#define LABEL_DEBUG_FUNCTION 3	/* function description */
#define LABEL_DEBUG_GLOBAL 4		/* global variable description */
#define LABEL_DEBUG_LOCAL 5			/* local variable description */
#define LABEL_DEBUG_PARAM 6			/* parameter description */
#define LABEL_DEBUG_FIELD 7			/* field description */
#define LABEL_DEBUG_TYPEDEF 8		/* typedef description */

/* union to hold any parameter value */
typedef union value {
	/* note: on a x86 processor, the assembler can't handle 
			variables > 64 bit */
	long long i;
	unsigned long long u;
	long double f;
} Value;

/* label information structure */
typedef struct label {
	char* name;				/* name */
	int segment;			/* segment (see instructions.h */
	int location;			/* location in segment */
	int id;						/* unique id */
	BOOL exported;		/* label is public */
	BOOL imported;		/* label is in other file */
	int debuginfo;		/* label is a debug label (see defines) */
	int accessed;			/* counter for how many times the label is accessed */
	struct label* ref;/* reference tracker for debug symbols 
												(debug symbol->label) */
} Label;

/* segment data structure */
typedef struct segment {
	int segment;			/* segment id (see instructions.h) */
	Memstream *data;	/* segment data */
	Memstream *mask;	/* segment label mask */
} Segment;

/* assembly data structure */
typedef struct assembly {
	Segment *code;		/* code segment data */
	Segment *lit;			/* lit segment data */
	Segment *data;		/* data segment data */
	int bss_size;			/* bss segment size */
	HashTable *labels;/* labels */
} Assembly;

/* load a file into an assembly structure */
Assembly* parse_file(FILE*);
/* create a new empty assembly structure */
Assembly* new_assembly();
/* parse a numeric value */
Value parse_value(char*, int, int);
/* print error to screeen and exit app */
void error(int, char*);
/* parse a valye type */
int parse_type(char);
/* write instruction to segment (assemble instruction) */
BOOL write_instruction(Assembly* assembly, char**, int, Segment*);
/* find a label in an assembly */
Label* get_label(Assembly*, char*);
/* find a label in an assembly by its id */
Label* get_label_from_id(Assembly*, int);
/* create a new label in an assembly */
Label* register_label(Assembly*, char*, int, int);
/* align a segment */
void align_segment(Segment*, int);
/* print all labels to screen (debug) */
void printLabels(Assembly* assembly);
/* print program usage */
void printUsage();
/* return segment name (debug */
char* get_segment_name(int segment);

#endif
