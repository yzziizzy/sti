#ifndef __sti__heap_h__
#define __sti__heap_h__

// Public Domain.

#include <stdint.h>
#include <sys/types.h> // ssize_t

typedef int (*comp_fn)(const void* a, const void* b, void* user);


// children at indices 2i + 1 and 2i + 2
// parent at index floor((i − 1) ∕ 2).

typedef struct heap_ {
	char* data;
	size_t len;
	size_t alloc;
	comp_fn cmp;
	void* user;
} heap_;

// declare a heap of type t
#define HEAP(t) \
union { \
	t** type; \
	heap_ heap; \
}

// initialize internals and set the cmp function
#define HEAP_init(h, c, u) \
do { \
	(h)->heap.cmp = (c); \
	(h)->heap.len = 0; \
	(h)->heap.alloc = 0; \
	(h)->heap.data = 0; \
	(h)->heap.user = (u); \
} while(0)

// insert a value into the heap
#define HEAP_insert(h, e) heap_insert_(&(h)->heap, (void*)(e), sizeof(*((h)->type)))
void heap_insert_(heap_* h, char* elem, size_t elem_sz);

// get the value of the most extreme value without removing it
#define HEAP_peek(h, e) heap_peek_(&(h)->heap, (void*)(e), sizeof(*((h)->type)))
int heap_peek_(heap_* h, char* out, size_t elem_sz);

// remove the top, most extreme value from the heap
#define HEAP_pop(h, e) heap_pop_(&(h)->heap, (void*)(e), sizeof(*((h)->type)))
int heap_pop_(heap_* h, char* out, size_t elem_sz);

// pop the top element and insert a new one at the same time
// faster than separate operations.
#define HEAP_insert_pop(h, i, o) heap_insert_pop_(&(h)->heap, (void*)(i), (o), sizeof(*((h)->type)))
void heap_insert_pop_(heap_* h, char* in, char* out, size_t elem_sz);


// free all internal memory. does not free h itself
#define HEAP_free(h) heap_free_(&(h)->heap);
void heap_free_(heap_* h);

#define HEAP_delete(h, e) heap_delete_(&(h)->heap, (void*)(e), sizeof(*((h)->type)))
void heap_delete_(heap_* h, char* elem, size_t elem_sz);

// returns -1 if not found, the item index otherwise
#define HEAP_find(h, e) heap_find_(&(h)->heap, (void*)(e), sizeof(*((h)->type)))
ssize_t heap_find_(heap_* h, char* elem, size_t elem_sz);


/*
int heap_contains_(heap_* h, char* elem, size_t elem_sz);
size_t heap_count_(heap_* h, char* elem, size_t elem_sz);
*/



#endif //__sti__heap_h__
