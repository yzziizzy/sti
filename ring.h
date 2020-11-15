#ifndef __sti__ring_h__
#define __sti__ring_h__

// Public Domain.


#include <stdlib.h> // size_t, calloc, free


// ----------------------------
// non-thread-safe ring buffers
// ----------------------------

// declare a ring buffer
#define RING(t) \
struct { \
	size_t len, alloc, first; \
	t* data; \
}

// initialisze a ring buffer
#define RING_INIT(x, sz) \
do { \
	(x)->data = calloc(1, sz * sizeof(*((x)->data))); \
	(x)->len = 0; \
	(x)->first = 0; \
	(x)->alloc = sz; \
} while(0)


// helpers
#define RING_LEN(x) ((x)->len)
#define RING_ALLOC(x) ((x)->alloc)
#define RING_DATA(x) ((x)->data)
#define RING_ITEM(x, i) (RING_DATA(x)[((x)->first + (i)) % (x)->alloc])

#define RING_TAIL(x) (RING_DATA(x)[((x)->first + (x)->len - 1) % (x)->alloc])
#define RING_HEAD(x) (RING_DATA(x)[(x)->first])

// #define RING_FIND(x, ptr_o) vec_find(RING_DATA(x), RING_LEN(x), sizeof(*RING_DATA(x)), ptr_o)

#define RING_TRUNC(x) \
do { \
	(x)->len = 0; \
	(x)->first = 0; \
while(0)
//  

#define RING_GROW(x) ring_resize((void**)&RING_DATA(x), &RING_ALLOC(x), sizeof(*RING_DATA(x)))

/*
// check if a size increase is needed to insert one more item
#define RING_CHECK(x) \
do { \
	if(RING_LEN(x) >= RING_ALLOC(x)) { \
		RING_GROW(x); \
	} \
} while(0)
*/

// operations

// assign new entry at the end
#define RING_PUSH(x, e) \
do { \
	if((x)->alloc > 0) { \
		if((x)->len >= (x)->alloc) { \
			(x)->data[((x)->first + (x)->len) % (x)->alloc] = (e); \
			(x)->first = ((x)->first + 1) % (x)->alloc; \
		} \
		else { \
			(x)->data[((x)->first + (x)->len) % (x)->alloc] = (e); \
			(x)->len++; \
		} \
	} \
} while(0)


// remove latest entry
#define RING_POP(x, e) \
do { \
	if((x)->len > 0 && (x)->alloc > 0) { \
		(e) = (x)->data[((x)->first + (x)->len - 1) % (x)->alloc]; \
		(x)->len--; \
	}  \
} while(0)





#define RING_PREPEND(x, e) \
do { \
	if((x)->alloc > 0) { \
		if((x)->len >= (x)->alloc) { \
			(x)->data[((x)->first + (x)->len) % (x)->alloc] = (e); \
			(x)->first = ((x)->first + 1) % (x)->alloc; \
		} \
		else { \
			(x)->data[((x)->first + (x)->len) % (x)->alloc] = (e); \
			(x)->len++; \
		} \
	} \
} while(0)


#define RING_PEEK(x) RING_DATA(x)[RING_LEN(x) - 1]


#define RING_POP1(x) \
do { \
	if((x)->len > 0 && (x)->alloc > 0) { \
		(x)->len--; \
	}  \
} while(0)


void ring_rm_(void* data, size_t stride, size_t* len, size_t* first, size_t alloc, size_t i);
#define RING_RM(x, i) \
ring_rm_((x)->data, sizeof(*((x)->data)), &((x)->len), &((x)->first), (x)->alloc, (i));


#define RING_FREE(x) \
do { \
	if(RING_DATA(x)) free(RING_DATA(x)); \
	(x)->data = 0; \
	(x)->len = 0; \
	(x)->first = 0; \
	(x)->alloc = 0; \
} while(0)

#define RING_COPY(copy, orig) \
do { \
	void* tmp; \
	tmp = realloc(RING_DATA(copy), RING_ALLOC(orig) * sizeof(*RING_DATA(orig)) ); \
	if(!tmp) { \
		fprintf(stderr, "Out of memory in ring copy"); \
		exit(1); \
	} \
	\
	RING_DATA(copy) = tmp; \
	RING_LEN(copy) = RING_LEN(orig); \
	RING_ALLOC(copy) = RING_ALLOC(orig); \
	(copy)->first = (orig)->first; \
	\
	memcpy(RING_DATA(copy), RING_DATA(orig),  RING_LEN(orig) * sizeof(*RING_DATA(orig))); \
} while(0)





/*
Loop macro magic

https://www.chiark.greenend.org.uk/~sgtatham/mp/

HashTable obj;
HT_LOOP(&obj, key, char*, val) {
	printf("loop: %s, %s", key, val);
}

effective source:

	#define HT_LOOP(obj, keyname, valtype, valname)
	if(0)
		finished: ;
	else
		for(char* keyname;;) // internal declarations, multiple loops to avoid comma op funny business
		for(valtype valname;;)
		for(void* iter = NULL ;;)
			if(HT_next(obj, iter, &keyname, &valname))
				goto main_loop;
			else
				while(1)
					if(1) {
						// when the user uses break
						goto finished;
					}
					else
						while(1)
							if(!HT_next(obj, iter, &keyname, &valname)) {
								// normal termination
								goto finished;
							}
							else
								main_loop:
								//	{ user block; not in macro }
*/
#define RING__PASTEINNER(a, b) a ## b
#define RING__PASTE(a, b) RING__PASTEINNER(a, b) 
#define RING__ITER(key, val) RING__PASTE(RING_iter_ ## key ## __ ## val ## __, __LINE__)
#define RING__FINISHED(key, val) RING__PASTE(RING_finished__ ## key ## __ ## val ## __, __LINE__)
#define RING__MAINLOOP(key, val) RING__PASTE(RING_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define RING_EACH(obj, index, valname) \
if(0) \
	RING__FINISHED(index, val): ; \
else \
	for(typeof(*RING_DATA(obj)) valname ;;) \
	for(size_t index = 0;;) \
		if(index < RING_LEN(obj) && (valname = RING_ITEM(obj, index), 1)) \
			goto RING__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto RING__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= RING_LEN(obj) || (valname = RING_ITEM(obj, index), 0)) { \
							goto RING__FINISHED(index, val); \
						} \
						else \
							RING__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }

// reverse
#define RING_R_EACH(obj, index, valname) \
if(0) \
	RING__FINISHED(index, val): ; \
else \
	for(typeof(*RING_DATA(obj)) valname ;;) \
	for(ptrdiff_t index = (ptrdiff_t)RING_LEN(obj) - 1;;) \
		if(index >= 0 && (valname = RING_ITEM(obj, index), 1)) \
			goto RING__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto RING__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(--index < 0 || (valname = RING_ITEM(obj, index), 0)) { \
							goto RING__FINISHED(index, val); \
						} \
						else \
							RING__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }



// this version only iterates the index   
#define RING_LOOP(obj, index) \
if(0) \
	RING__FINISHED(index, val): ; \
else \
	for(size_t index = 0;;) \
		if(index < RING_LEN(obj)) \
			goto RING__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto RING__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= RING_LEN(obj)) { \
							goto RING__FINISHED(index, val); \
						} \
						else \
							RING__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }

// reverse; this version only iterates the index   
#define RING_R_LOOP(obj, index) \
if(0) \
	RING__FINISHED(index, val): ; \
else \
	for(ptrdiff_t index = (ptrdiff_t)RING_LEN(obj) - 1;;) \
		if(index >= 0) \
			goto RING__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto RING__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(--index < 0) { \
							goto RING__FINISHED(index, val); \
						} \
						else \
							RING__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }







#endif // __sti__ring_h__
