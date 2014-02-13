/*
 * The segments.c file contains the code which defines the addresses of all 
 * 		the segments within an executable. This file can be changed if the
 * 		executable layout needs to be changed (for example, if there is 
 * 		read-only memory available, the code/lit segments can be mapped to this,
 * 		while the data/bss segments must be mapped to writable memory.
 * 
 * One rule applies: the code segment should ALWAYS start at address 0, since
 * 		the X32 always starts executing from 0.
 * One reccommendation: if the BSS segment is placed at the end of an 
 * 		executable, it is not included in the destination file (which makes the
 * 		executable file a lot smaller.
 *
 *	Author: Sijmen Woutersen
 */

/* this function computes the segment offsets. The five parameters contain the
 *		segment sizes, in order: overlap, code, lit, data, bss. The overlap
 *		segment is not included in the executable, and is created by the -base
 *		argument. All segments should be placed beyond this 'fictional' segment.
 *		The last 4 parameters are pointers to the 4 segment offsets.
 */
void get_segment_offsets(int, int, int, int, int, int*, int*,	int*, int*);
