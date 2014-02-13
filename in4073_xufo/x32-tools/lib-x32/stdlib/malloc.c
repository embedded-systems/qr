#include <stdlib.h>

extern char malloc_memory;
extern unsigned malloc_memory_size;

#define MSB 0x80000000

char *_memory = 0;
char *_onepastmemory = 0;

void *_freeblockofsize(unsigned);
int _coalescefreechunks();

void* malloc(size_t size) {
	void* ptr;

	/* initialize */
	if (!_memory) {
		_memory = &malloc_memory;
		*(unsigned*)_memory = MSB | malloc_memory_size;
		_onepastmemory = _memory + malloc_memory_size;
	}

	/* do not allow allocating 0 bytes */
	if (size == 0) return 0;

	/* try to find a free block */
	if (ptr = _freeblockofsize(size)) return ptr;
	/* failed, coalesce the free chuncks */
	if (_coalescefreechunks()) {
		/* try again, return the result, even if it's 0 */
		return _freeblockofsize(size);
	} else {
		/* no extra free space was generated, sorry */
		return 0;
	}
}

void free(void* data) {
	*((unsigned*)data-1) |= MSB;
}

void* _freeblockofsize(size_t size) {
	char *chunk = _memory;
	unsigned requestedsize = size+sizeof(unsigned);

	/* cannot use MSB of size (would be +/-2.7gig on 32-bit system) */
	if (size & MSB) return 0;

	while (chunk < _onepastmemory) {
		if (*(unsigned*)chunk > ((requestedsize + sizeof(unsigned)) | MSB)) {
			/* large enough to hold data and split */
			
			/* create next chunk header */
			*(unsigned*)(chunk + requestedsize) = (*(unsigned*)chunk - requestedsize);
			/* make current chunk smaller & free */
			*(unsigned*)chunk = requestedsize & ~MSB;

			return chunk+sizeof(unsigned);
		} else if (*(unsigned*)chunk > (requestedsize | MSB)) {
			/* large enough to hold data, but not to split */
			*(unsigned*)chunk &= ~MSB;
			return chunk + sizeof(unsigned);
		} else {
			/* chunk not large enough (or not free) */
			chunk += *(unsigned*)chunk & ~MSB;
		}
	}
	return 0;
}

int _coalescefreechunks() {
	char *chunk = _memory;
	int ret = 0;
	
	while ((chunk+(*(unsigned*)chunk & ~MSB)) < _onepastmemory) {
		if ((*(unsigned*)chunk) & (*(unsigned*)(chunk+(*(unsigned*)chunk & ~MSB))) & MSB) {
			/* coalesce with next chunk */
			*(unsigned*)chunk += (*(unsigned*)(chunk+(*(unsigned*)chunk & ~MSB)) & ~MSB);
			/* return 1 */
			ret = 1;
		} else {
			/* current or next chunk not free */
			chunk += *(unsigned*)chunk & ~MSB;
		} 
	}
	return ret;
}

