
#include <string.h>


#include "heap.h"



static inline void swap_mem_(char* a, char* b, size_t elem_sz) {
	char* t = alloca(elem_sz);
	memcpy(t, a, elem_sz);
	memcpy(a, b, elem_sz);
	memcpy(b, t, elem_sz);
}

void heap_up_(heap_* h, size_t pi, size_t elem_sz);
void heap_down_(heap_* h, size_t pi, size_t elem_sz);


void heap_resize_(heap_* h, size_t new_size, size_t elem_sz) {
	if(!h->data) {
		h->data = malloc(elem_sz * 16);
		h->alloc = 16;
	}
	else {
		h->data = realloc(h->data, elem_sz * new_size);
		h->alloc = new_size;
	}
}





void heap_insert_(heap_* h, char* elem, size_t elem_sz) {
	if(h->len >= h->alloc) heap_resize_(h, h->alloc * 2, elem_sz);
	
	// insert the element at the end
	memcpy(h->data + (elem_sz * h->len), elem, elem_sz);
	
	h->len++;
	
	heap_up_(h, h->len-1, elem_sz);
}


int heap_peek_(heap_* h, char* out, size_t elem_sz) {
	if(h->len == 0) {
		return 1;
	}
	// copy the top element
	memcpy(out, h->data, elem_sz);
	
	return 0;
}


int heap_pop_(heap_* h, char* out, size_t elem_sz) {
	if(h->len == 0) {
		return 1;
	}
	
	// copy the top element
	memcpy(out, h->data, elem_sz);
	
	// swap
	swap_mem_(h->data + ((h->len-1) * elem_sz), h->data, elem_sz);
	
	h->len--;
	
	heap_down_(h, 0, elem_sz);
	
	return 0;
}

void heap_insert_pop_(heap_* h, char* in, char* out, size_t elem_sz) {
	
	// copy the top element out and the new element in
	if(h->len == 0) {
		memcpy(out, h->data, elem_sz);
		h->len++;
	}
	
	memcpy(h->data, in, elem_sz);
	
	heap_down_(h, 0, elem_sz);
}


void heap_delete_(heap_* h, char* elem, size_t elem_sz) {

	// find the item to be deleted
	ssize_t ind = heap_find_(h, elem, elem_sz);
	if(ind == -1) return;
	
	// swap with last item, then delete the new last item
	swap_mem_(h->data + (ind * elem_sz), h->data + ((h->len-1) * elem_sz), elem_sz);
		
	// rebalance heap with the new value in the deletion location
	heap_down_(h, ind, elem_sz);
}


void heap_free_(heap_* h) {
	if(h->data) free(h->data);
	h->data = NULL;
	h->len = 0;
	h->alloc = 0;
}




void heap_up_(heap_* h, size_t ci, size_t elem_sz) {
	while(ci != 0) {
		
		size_t pi = (ci - 1) / 2;
		
		int ret = h->cmp(h->data + (elem_sz * pi), h->data + (elem_sz * ci), h->user);
		if(ret > 0) break;
		
		swap_mem_(h->data + (elem_sz * pi), h->data + (elem_sz * ci), elem_sz);
		
		ci = pi;
	}
}


void heap_down_(heap_* h, size_t pi, size_t elem_sz) {
	size_t ai = (pi * 2) + 1;
	size_t bi = (pi * 2) + 2;
	size_t ci;
	int r;
	
	if(ai >= h->len) return;
	if(bi >= h->len) {
		ci = ai; // only one child
	}
	else {
		// find the smallest child
		r = h->cmp(h->data + (ai * elem_sz), h->data + (bi * elem_sz), h->user);
		ci =  r > 0 ? ai : bi; 
	}
	
// 	printf("smaller: %d of %d and %d\n", *(int*)(h->data+ci*elem_sz),*(int*)(h->data+ai*elem_sz),*(int*)(h->data+bi*elem_sz));
	
	r = h->cmp(h->data + (ci * elem_sz), h->data + (pi * elem_sz), h->user);
	if(r > 0) {
// 		printf(" -> %d < %d, swapping \n", *(int*)(h->data+ci*elem_sz),*(int*)(h->data+pi*elem_sz));
		
		swap_mem_(h->data + (ci * elem_sz), h->data + (pi * elem_sz), elem_sz);
		
		heap_down_(h, ci, elem_sz);
		return;
	}
	
}


ssize_t heap_find_(heap_* h, char* elem, size_t elem_sz) {
	int r;
	
	// TODO: replace with binary search, assuming it's actually faster
	
	for(size_t i = 0; i < h->len; i++) {
		if(0 == memcmp(h->data + i * elem_sz, elem, elem_sz)) return i;
	}
	
	return -1;
}


