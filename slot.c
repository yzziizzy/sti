
// Public Domain

#include "slot.h"



void slot_resize(struct slot_base_props* base, size_t chunk_mem_size, size_t chunk_len) {
	
	if(base->chunksLen >= base->chunksAlloc) {
		if(base->chunksAlloc == 0) base->chunksAlloc = 4;
		else base->chunksAlloc *= 2;
		
		base->data = realloc(base->data, base->chunksAlloc * sizeof(void*));
	}
	
	base->data[base->chunksLen++] = calloc(1, chunk_mem_size);
	base->alloc += chunk_len;
}




// returns 1 to continue, 0 top stop
int slot_next(struct slot_base_props* base, u64 chunkLen, u64* c, u64* i, int inc) {
	
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




