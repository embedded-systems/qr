/* see memstream.h for general information */
#include "memstream.h"
#include <stdlib.h>
#include <string.h>

/* enlarge a memory stream (factor 2) */
BOOL enlarge(Memstream*);

/*
 * create a new memory stream
 */
Memstream* MS_Open(int initialsize) {
	Memstream* ms;	/* info structure */
	
	/* allocate memory for info structure */
	ms = malloc(sizeof(Memstream));
	if (ms == 0) return 0;
	
	/* allocate memory for actual data */
	ms->data = malloc(initialsize);
	if (ms->data == 0) {
		free(ms);
		return 0;
	}
	
	/* initialize variables */
	ms->byteptr = 0;
	ms->bitptr = 0;
	ms->size = 0;
	ms->allocated = initialsize;
	
	/* return info structure */
	return ms;
}

/*
 * close a memory stream
 */
void MS_Close(Memstream* memstream) {
	/* free all data */
	free(memstream->data);
	free(memstream);
}

/*
 * write data to a memory stream
 */
BOOL MS_Write(Memstream* memstream, void* data, int size) {
	/* try to enlarge the memory stream as much as needed */
	while (size + memstream->byteptr > memstream->allocated) {
		if (!enlarge(memstream)) return FALSE;
	}
	/* copy the data to the memory stream */
	if (data) memcpy(memstream->data+memstream->byteptr, data, size);
	/* update bytepointer */
	memstream->byteptr += size;
	/* update size */
	if (memstream->byteptr > memstream->size) memstream->size = memstream->byteptr;
	/* reset bitpointer when writing bytes */
	memstream->bitptr = 0;
	return TRUE;
}


/*
 *	write big endian value to memorystream
 */
BOOL MS_WriteBE(Memstream* memstream, long long value, int size) {
	int i;
	/* try to enlarge the memory stream as much as needed */
	while (size + memstream->byteptr > memstream->allocated) {
		if (!enlarge(memstream)) return FALSE;
	}
	
	/* copy the data to the memory stream */
	for (i = 0; i < size; i++) {
		((unsigned char*)memstream->data)[memstream->byteptr + size - 1 - i] = value & 0xFF;
		value >>= 8;
	}
	/* update bytepointer */
	memstream->byteptr += size;
	/* update size */
	if (memstream->byteptr > memstream->size) memstream->size = memstream->byteptr;
	/* reset bitpointer when writing bytes */
	memstream->bitptr = 0;
	return TRUE;
}

/*
 *	align a memorystream
 */
BOOL MS_Align(Memstream* memstream, int alignment) {
	/* write zeros until alignment is ok */
	while (memstream->size % alignment) {
		MS_Write(memstream, 0, 1);
	}
}

/*
 * double the size of a memory stream
 */
BOOL enlarge(Memstream* memstream) {
	void* newptr;	/* pointing to new memory block */
	void* oldptr;	/* pointing to old memory block */
	/* make sure to increase (0->2) */
	if (memstream->allocated == 0) memstream->allocated = 1;

	/* save old pointer */
	oldptr = memstream->data;

	/* allocate new block of memory */
	newptr = malloc(memstream->allocated<<1);
	if (!newptr) return FALSE;
	/* store new pointer in info structure */
	memstream->data = newptr;

	/* save old data */
	memcpy(newptr, oldptr, memstream->size); 

	/* free old block */
	free(oldptr); 

	/* update allocated value */
	memstream->allocated = memstream->allocated<<1;
	return TRUE;
}

/*
 * copy a memory stream to a file stream
 */
void MS_Dump(Memstream *memstream, FILE* file) {
	unsigned char* byte;			/* pointer to current byte */
	unsigned char* lastbyte;	/* pointer to last byte */
	
	/* find last byte */
	lastbyte = memstream->data+memstream->size;
	
	/* copy */
	for (byte = memstream->data; byte < lastbyte; byte++) {
		fputc(*byte, file);
	}
}

/*
 * write a bit to a memory stream 
 */
void MS_WriteBit(Memstream* memstream, BOOL value) {
	unsigned char* ptr;	/* ptr to byte */
	
	/* see if there is a byte free */
	if (memstream->byteptr == memstream->allocated) {
		enlarge(memstream);
	}
	/* start a new byte if necessary */
	if (memstream->size == memstream->byteptr) {
		/* update size */
		memstream->size++;
		ptr = (unsigned char*)(memstream->data+memstream->byteptr);
		/* reset byte */
		*ptr = 0;
		memstream->bitptr = 0;
	} else {
		ptr = (unsigned char*)(memstream->data+memstream->byteptr);
	}
	
	/* if writing a 1 */
	if (value) {
		/* update byte */
		*ptr |= 1<<(7-memstream->bitptr);
	}
	/* update bitpointer, reset to 0 if necessary */
	memstream->bitptr++;
	if (memstream->bitptr == 8) {
		memstream->byteptr++;
		memstream->bitptr = 0;
	}
}

/*
 * seek the memory stream 
 */
void MS_Seek(Memstream* memstream, int position) {
	/* update position */
	memstream->byteptr = position;
	memstream->bitptr = 0;
}

/*
 * read from the memory stream
 */
int MS_Read(Memstream* memstream, void* data, int size) {
	int bytes;	/* nr of bytes read */
	/* check if the stream is large enough to read all bytes */
	if (size + memstream->byteptr > memstream->size) {
		/* no; set bytes to max available */
		bytes = memstream->size - memstream->byteptr;
	} else {
		/* yes */
		bytes = size;
	}
	
	/* copy memory */
	memcpy(data, memstream->data + memstream->byteptr, bytes);
	/* update byte & bitpointer */
	memstream->byteptr += bytes;
	memstream->bitptr = 0;
	return bytes;
}

/*
 * read big endian value from memory stream
 */
long long MS_ReadBE(Memstream* memstream, int size) {
	unsigned char* data; 	/* raw data */
	int i;								/* loop variable */
	long long ret;				/* return value */
	/* allocate data for raw memory */
	data = (unsigned char*)malloc(size);
	ret = 0;
	/* read raw memory (converting directly from memory would be faster) */
	MS_Read(memstream, data, size);
	
	/* convert to long long */
	for (i = 0; i < size; i++) {
		ret = ret<<8;
		ret += data[i];
	}
	/* cleanup */
	free(data);
	
	/* return */
	return ret;
}

/*
 * read a bit from the memory stream
 */
BOOL MS_ReadBit(Memstream* memstream) {
	BOOL ret;	/* return value */
	/* see if a bit can be read */
	if (memstream->byteptr >= memstream->size) {
		return FALSE;
	} else {
		/* read current byte, check if the bit is set */
		if (((unsigned char*)memstream->data)[memstream->byteptr] & (1<<(7-memstream->bitptr))) {
			ret = TRUE;
		} else {
			ret = FALSE;
		}
		
		/* update bit and byte pointers */
		memstream->bitptr++;
		if (memstream->bitptr == 8) {
			memstream->bitptr = 0;
			memstream->byteptr++;
		}
		
		/* return */
		return ret;
	}
}

/*
 * load a file into a memory stream
 */
Memstream* MS_Load(FILE* file, int size) {
	Memstream* memstream;
	/* create */
	memstream = MS_Open(size);
	/* read */
	fread(memstream->data, size, 1, file);
	/* set size */
	memstream->size = memstream->allocated;
	/* return */
	return memstream;
}
