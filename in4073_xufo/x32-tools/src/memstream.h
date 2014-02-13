/*
 * Memstream: contains functions to use memory as a stream (eg file).
 *
 * 		Use MS_Open to create a new memory stream (base size required), 
 * 		MS_Close, to close the stream. To read from or write to a memory 
 * 		stream use MS_Read and MS_Write just like fread and fwrite.
 *
 * 		The memory block is automatically increased when neccessary, each
 *		time with a factor 2.
 *
 * 		In addition, there are also functions to read and write bits, and
 *		to read and write big endian values
 *
 *	Author: Sijmen Woutersen
 *
 */
 
#ifndef MEMSTREAM_H
#define MEMSTREAM_H
#include "bool.h"
#include <stdio.h>

/* memory stream information structure */
typedef struct memstream {	
	void* data;			/* pointer to memory block */
	int size;				/* current size of stream */
	int allocated;	/* amount of bytes allocated in memory block
											(size can't be larger than allocated */
	int byteptr;		/* pointer to the current byte to read/write */
	int bitptr;			/* pointer to the current bit to read/write */
} Memstream;

/* open a new memory stream */
Memstream* MS_Open(int);
/* close a memory stream */
void MS_Close(Memstream*);
/* write data to a memory stream */
BOOL MS_Write(Memstream*, void*, int);
/* read data from a memory stream */
int MS_Read(Memstream*, void*, int);
/* write big endian value to a memory stream */
BOOL MS_WriteBE(Memstream*, long long, int);
/* read a big endian value from a memory stream */
long long MS_ReadBE(Memstream*, int);
/* dump memory stream to file */
void MS_Dump(Memstream*, FILE*);
/* load memory stream from file */
Memstream* MS_Load(FILE*, int);
/* write a bit to a memory stream */
void MS_WriteBit(Memstream*, BOOL);
/* read a bit from a memory stream */
BOOL MS_ReadBit(Memstream*);
/* seek a memory stream */
void MS_Seek(Memstream*, int);
/* align a memory stream */
BOOL MS_Align(Memstream*, int);
#endif
