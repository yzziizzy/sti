

#include "svec.h"



void svec_resize(struct svec_base_props* base, size_t elem_size, size_t chunk_size) {
	
	if(base->chunksLen >= base->chunksAlloc) {
		if(base->chunksAlloc == 0) base->chunksAlloc = 16;
		else base->chunksAlloc *= 2;
		
		base->data = realloc(base->data, base->chunksAlloc * sizeof(void*));
	}
	
	base->data[base->chunksLen++] = malloc(chunk_size * elem_size);
	base->alloc += chunk_size;
}



long svec_pointer_index(struct svec_base_props* base, size_t elem_size, size_t chunk_size, void* ptr) {
	
	for(long c = 0; c < base->chunksLen; c++) {
		if(ptr >= base->data[c] && ptr <= base->data[c] + (chunk_size * elem_size)) {
			return ((ptr - base->data[c]) / elem_size) + (c * chunk_size * elem_size);
		}
	}
	
	return -1;
}





void svec_free(struct svec_base_props* base) {
	
	for(long i = 0; i < base->chunksLen; i++) {
		free(base->data[i]);
	} 
	
	free(base->data);
	
	base->chunksAlloc = 0;
	base->chunksLen = 0;
	base->len = 0;
	base->alloc = 0;
}




