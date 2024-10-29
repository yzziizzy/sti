// Public Domain.


#include <stdlib.h> // realloc
#include <string.h> // memcmp
#include <stdio.h> // fprintf

#include "vec.h"


// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline static size_t sti_vec_nextPOT(size_t in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}



void vec_resize_to(void** data, size_t* size, size_t elem_size, size_t new_size) {
	void* tmp;
	
	if(*size >= new_size) return;
	
	*size = sti_vec_nextPOT(new_size);
	
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
	
	size_t npot = sti_vec_nextPOT(new_size);
	
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


// IMPORTANT: the vec is assumed to be sorted except the specified index
ssize_t vec_bubble_index(void* data, size_t len, size_t stride, size_t index, int (*cmp)(const void*,const void*)) {
	#define signum(x) ((x > 0) - (x < 0))
	// find the destination index
	
	// TEMP: linear search for now
	ssize_t dst_index = index;
	
	if(len == 0) return -1;
	
	// there are three scenarios:
	//   index is the first element, so only search forward.
	//   index is the last element, so only search backward.
	//   index is in the middle, so we need to decide which direction to go
	
	int direction;
	
	if(index == 0) {
		direction = 1;
	} 
	else if(index >= len - 1) {
		direction = -1;
	}
	else {
		int res_p = cmp(data + ((index + 1) * stride), data + (index * stride));
		int res_m = cmp(data + ((index - 1) * stride), data + (index * stride));
		
		// if either is 0, index is properly sorted already
		if(res_p == 0 || res_m == 0) return index;
		if(res_p >= 0 && res_m <= 0) return index; // already sorted
		
		direction = res_p > 0 ? -1 : 1;
	}
	
//	dst_index += direction;
	 do {
		dst_index += direction;
		
		int res = cmp(data + ((dst_index) * stride), data + (index * stride));
		
		if(res == 0 || signum(res) == direction) break;
		
		
	} while(dst_index > 0 && dst_index < len - 1);
	
	// TODO: sanity checks on degenerate moves
	void* tmp = alloca(stride);
	memcpy(tmp, data + (index * stride), stride);
	
	if(dst_index > index) { // move intermediate values backwards
		memmove(data + (index * stride), data + ((index + 1) * stride), stride * (dst_index - index));
	} 
	else if(dst_index < index) { // move intermediate values forwards
		memmove(data + ((dst_index + 1) * stride), data + (dst_index * stride), stride * (index - dst_index));
	}
	else {
		// no move; already sorted
	}
	
	memcpy(data + (dst_index * stride), tmp, stride);
	
	return dst_index;
}



void vec_uniq(void* data, size_t* lenp, size_t stride, int (*cmp)(const void*,const void*)) {
	size_t read_index = 0;
	size_t write_index = 0;
	size_t len = *lenp;
	
	while(read_index < len) {
		memcpy(data + (write_index * stride), data + (read_index * stride), stride);
		
		do {
			read_index++;
		} while(read_index < len && 0 == cmp(data + (write_index * stride), data + (read_index * stride)));
		
		write_index++;
	}

	*lenp = write_index;
}

void vec_uniq_r(void* data, size_t* lenp, size_t stride, int (*cmp)(const void*,const void*,void*), void* arg) {
	size_t read_index = 0;
	size_t write_index = 0;
	size_t len = *lenp;
	
	while(read_index < len) {
		memcpy(data + (write_index * stride), data + (read_index * stride), stride);
		
		do {
			read_index++;
		} while(read_index < len && 0 == cmp(data + (write_index * stride), data + (read_index * stride), arg));
		
		write_index++;
	}

	*lenp = write_index;
}
