
// Public Domain

#include <limits.h>
#include "slot.h"

// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline static size_t slot_nextPOT_(size_t in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}

void slot_resize(struct slot_base_props* base, size_t chunk_mem_size, size_t chunk_len) {
	
	if(base->fill == 0 && base->alloc == 0) {
		base->nextFree = ULONG_MAX; // initialize the free list's tail
	}
	
	if(base->chunksLen >= base->chunksAlloc) {
		if(base->chunksAlloc == 0) base->chunksAlloc = 4;
		else base->chunksAlloc *= 2;
		
		base->data = realloc(base->data, base->chunksAlloc * sizeof(void*));
	}
	
	base->data[base->chunksLen++] = calloc(1, chunk_mem_size);
	base->alloc += chunk_len;
}


void slot_resize_to(struct slot_base_props* base, size_t chunk_mem_size, size_t chunk_len, size_t min_size) {
	
	if(base->fill == 0 && base->alloc == 0) {
		base->nextFree = ULONG_MAX; // initialize the free list's tail
	}
	
	uint64_t needed_chunks = (min_size / chunk_len) + 1;
	
	if(needed_chunks >= base->chunksAlloc) {
		base->chunksAlloc = slot_nextPOT_(needed_chunks);
		if(base->chunksAlloc < 4) base->chunksAlloc = 4;
		
		base->data = realloc(base->data, base->chunksAlloc * sizeof(void*));
		
		for(int i = base->chunksLen; i < base->chunksAlloc; i++) base->data[i] = NULL;
	}
	
	// allocate memory up to the requested size
	if(needed_chunks > base->chunksLen) {
		for(int i = base->chunksLen; i < needed_chunks; i++) base->data[i] = calloc(1, chunk_mem_size);
		base->chunksLen = needed_chunks;
	}
	
	base->alloc = base->chunksLen * chunk_len;
}



// returns 1 to continue, 0 top stop
int slot_next(struct slot_base_props* base, uint64_t chunkLen, uint64_t* c, uint64_t* i, int inc) {
	
	if(inc) goto INC;
	
	while(1) {
		
		uint64_t* occ = base->data[*c];
		if(occ[*i / 64] & (1ul << (*i % 64))) {
			// found
			return 1;
		}
		
	INC:
		
		(*i)++;
		if(*i >= chunkLen) {
			*i = 0;
			(*c)++;
		}	
	
		if(*c >= base->chunksLen) {
			// end of array;
			return 0;
		}
	}
	
	return 0;
}





void slot_free(struct slot_base_props* base) {
	
	for(long i = 0; i < base->chunksLen; i++) {
		free(base->data[i]);
	} 
	
	free(base->data);
	
	base->chunksAlloc = 0;
	base->chunksLen = 0;
	base->fill = 0;
	base->alloc = 0;
	base->nextFree = 0;
}



