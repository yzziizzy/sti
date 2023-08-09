#ifndef __sti__svec_h__
#define __sti__svec_h__

#include <stddef.h>
#include <stdlib.h>

/*

SVEC stands for Stable Vector. Pointers to indices will never change and can be cached forever

*/

#define SVEC_CHUNK_SIZE(x) 512

struct svec_base_props {
	void** data; // overlaps with the data pointer in the union
	size_t len, alloc; \
	size_t chunksAlloc; \
	size_t chunksLen; \
};

void svec_resize(struct svec_base_props* base, size_t elem_size, size_t chunk_size);
long svec_pointer_index(struct svec_base_props* base, size_t elem_size, size_t chunk_size, void* ptr);
void svec_free(struct svec_base_props* base);

#define SVEC(type) \
	union { \
		type** chunks; \
		struct svec_base_props b; \
	}


// initialisze a vector
#define SVEC_init(x) \
do { \
	(x)->chunks = NULL; \
	(x)->b.len = 0; \
	(x)->b.alloc = 0; \
	(x)->b.chunksAlloc = 0; \
	(x)->b.chunksLen = 0; \
} while(0)


// helpers
#define SVEC_len(x) ((x)->b.len)
#define SVEC_item(x, i) ((x)->chunks[(i) / SVEC_CHUNK_SIZE(x)][(i) % SVEC_CHUNK_SIZE(x)])

#define SVEC_itemp(x, i) ({ \
	typeof(i) __SVEC_tmp = (i); \
	&((x)->chunks[(__SVEC_tmp) / SVEC_CHUNK_SIZE(x)][(__SVEC_tmp) % SVEC_CHUNK_SIZE(x)]); \
})

#define SVEC_tail(x) (SVEC_item((x)->b.len - 1))
#define SVEC_head(x) (SVEC_item(0))
#define SVEC_push(x, y) \
do { \
	if((x)->b.len <= (x)->b.alloc) { \
		svec_resize(&(x)->b, sizeof(**(x)->chunks), SVEC_CHUNK_SIZE(x)); \
	} \
	*SVEC_itemp((x), (x)->b.len++) = (y); \
} while(0);


#define SVEC_inc(x) \
({ \
	if((x)->b.len <= (x)->b.alloc) { \
		svec_resize(&(x)->b, sizeof(**(x)->chunks), SVEC_CHUNK_SIZE(x)); \
	} \
	memset(SVEC_itemp((x), (x)->b.len), 0, sizeof(**(x)->chunks)); \
	SVEC_itemp((x), (x)->b.len++); \
})

#define SVEC_pointer_index(x, p) svec_pointer_index(&(x)->b, sizeof(**(x)->chunks), SVEC_CHUNK_SIZE(x), (p));



#define SVEC_free(x) svec_free(&(x)->b); 



#define SVEC_EACH _Pragma("GCC error \"'SVEC_EACH' does not exist. Use 'SVEC_EACHP'.\"")


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
// pointer version
#define SVEC__PASTE(a, b) CAT(a, b) 
#define SVEC__ITER(key, val) SVEC__PASTE(SVEC_iter_ ## key ## __ ## val ## __, __LINE__)
#define SVEC__FINISHED(key, val) SVEC__PASTE(SVEC_finished__ ## key ## __ ## val ## __, __LINE__)
#define SVEC__MAINLOOP(key, val) SVEC__PASTE(SVEC_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define SVEC_EACHP(obj, index, valname) \
if(0) \
	SVEC__FINISHED(index, val): ; \
else \
	for(typeof(*(obj)->chunks) valname ;;) \
	for(size_t index = 0;;) \
		if(index < SVEC_len(obj) && (valname = SVEC_itemp(obj, index), 1)) \
			goto SVEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto SVEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= SVEC_len(obj) || (valname = SVEC_itemp(obj, index), 0)) { \
							goto SVEC__FINISHED(index, val); \
						} \
						else \
							SVEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }





#endif // __sti__svec_h__
