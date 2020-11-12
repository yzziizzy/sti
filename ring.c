#include <stddef.h>
#include <string.h>


#include "ring.h"


void ring_rm_(void* data, size_t stride, size_t* len, size_t* first, size_t alloc, size_t i) {
	
	if(*len == 0) return;
	
	size_t end = (*first + *len - 1) % alloc;
	size_t n = (*first + i) % alloc;
	
	// simple case for removing the last element	
	if(n == *len) {
		*len = ((*len - 1) + alloc) % alloc;
		return;			
	}
	
	// simple case for removing the first element	
	if(n == *first) {
		*first = (*first + 1) % alloc;
		(*len)--;
		return;			
	}
	
	if(end < *first) { // the data wraps around
		if(n < end) { // easy case; move the end data backward
			void* dst = data + (n * stride);	
			void* src = data + ((n + 1) * stride);
			
			memmove(dst, src, (end - n) * stride);
		}
		else if(n > *first) { // both ends must be adjusted
			
			// move the tail
			memmove(
				data + (n * stride), 
				data + ((n + 1) * stride), 
				(alloc - n - 1) * stride
			);
			// copy the head to the tail
			memcpy(
				data + ((alloc - 1) * stride), 
				data, 
				stride
			);
			// move the wrapped end back
			memmove(
				data, 
				data + stride, 
				(end) * stride
			);
		}
	}
	else { // all the data is sequential
		void* dst = data + (n * stride);		
		void* src = data + ((n + 1) * stride);		
	
		memmove(dst, src, (*len - n - 1) * stride);
	}
	
	(*len)--;
}
