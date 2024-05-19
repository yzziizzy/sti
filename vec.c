// Public Domain.


#include <stdlib.h> // realloc
#include <string.h> // memcmp
#include <stdio.h> // fprintf

#include "vec.h"


#ifndef STI_HAS_STATIC_nextPOT
#define STI_HAS_STATIC_nextPOT
// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline static size_t nextPOT(size_t in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}
#endif

void vec_resize_to(void** data, size_t* size, size_t elem_size, size_t new_size) {
	void* tmp;
	
	if(*size >= new_size) return;
	
	*size = nextPOT(new_size);
	
	tmp = realloc(*data, *size * elem_size);
	if(!tmp) {
		fprintf(stderr, "Out of memory in vector resize, %ld bytes requested\n", *size);
		return;
	}
	
	*data = tmp;
}

void vec_c_resize_to(void** data, size_t* size, size_t elem_size, size_t new_size) {
	void* tmp;
	
	if(*size >= new_size) return;
	
	size_t npot = nextPOT(new_size);
	
	tmp = realloc(*data, npot * elem_size);
	if(!tmp) {
		fprintf(stderr, "Out of memory in vector resize, %ld bytes requested\n", npot);
		return;
	}
	
	memset(tmp + *size * elem_size, 0, (npot - *size) * elem_size);
	
	*size = npot;
	*data = tmp;
}

void vec_resize(void** data, size_t* size, size_t elem_size) {
	void* tmp;
	
	if(*size < 8) *size = 8;
	else *size *= 2;
	
	tmp = realloc(*data, *size * elem_size);
	if(!tmp) {
		fprintf(stderr, "Out of memory in vector resize");
		return;
	}
	
	*data = tmp;
}
 
ptrdiff_t vec_find(void* data, size_t len, size_t stride, void* search) {
	size_t i;
	for(i = 0; i < len; i++) {
		if(0 == memcmp((char*)data + (i * stride), search, stride)) {
			return i;
		}
	}
	
	return -1;
}
 
ptrdiff_t vec_rm_val(char* data, size_t* len, size_t stride, void* search) {
	size_t i;
	for(i = 0; i < *len; i++) {
		if(0 == memcmp(data + (i * stride), search, stride)) {
			if(i < *len) {
				memcpy(data + (i * stride), data + ((*len - 1) * stride), stride);
			}
			(*len)--;
			return 0;
		}
	}
	
	return 1;
}

void vec_copy(
	char** dst_data, char* src_data, 
	size_t* dst_alloc, size_t src_alloc, 
	size_t* dst_len, size_t src_len, 
	size_t elem_size
) {
	if(*dst_alloc < src_alloc) {
		*dst_alloc = src_alloc;
		if(*dst_data == 0) {
			*dst_data = malloc(elem_size * src_alloc);
		}
		else {
			*dst_data = realloc(*dst_data, elem_size * src_alloc);
		}
	}
	
	*dst_len = src_len;
	memcpy(*dst_data, src_data, elem_size * *dst_len);
}
