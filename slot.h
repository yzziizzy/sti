#ifndef __sti__slot_h__
#define __sti__slot_h__

// Public Domain

#include <stddef.h>
#include <stdint.h>

/*

Slot vector; a stable vector that tracks empty indices.

Pointers to indices will never change and can be cached forever.

Minimum item size is 64 bits.

*/



struct slot_base_props {
	void** data; // overlaps with the chunk pointer in the union
	size_t fill, alloc; // total count of all items, and total space allocated across all chunks
	size_t chunksAlloc; // how big the chunk pointer array is (data)
	size_t chunksLen; // how many chunks are actually allocated
	uint64_t nextFree;
};


void slot_resize(struct slot_base_props* base, size_t chunk_mem_size, size_t chunk_len);
void slot_resize_to(struct slot_base_props* base, size_t chunk_mem_size, size_t chunk_len, size_t min_size);
int slot_next(struct slot_base_props* base, uint64_t chunkLen, uint64_t* c, uint64_t* i, int inc);
void slot_free(struct slot_base_props* base);

#define SLOT(type, chunklen) \
	union { \
		struct { \
			uint64_t occupancy[((chunklen) / 64) + !!((chunklen) % 64)]; \
			union { \
				type t; \
				uint64_t nextFree; \
			} data[chunklen]; \
		}** chunks; \
		struct slot_base_props b; \
		struct { \
			type* tp; \
			type t; \
			uint8_t chunk_len[chunklen]; \
		} meta[0]; \
	}


// initialize a slot, completely optional; zero-fill is initialized
#define SLOT_init(x) \
do { \
	(x)->chunks = NULL; \
	(x)->b.fill = 0; \
	(x)->b.alloc = 0; \
	(x)->b.chunksAlloc = 0; \
	(x)->b.chunksLen = 0; \
	(x)->b.nextFree = 0; \
} while(0)

//
// internal helpers
//

#define SLOT_ITEM_SIZE(x) (sizeof((x)->meta[0].t))

// length in items
#define SLOT_CHUNK_LEN(x) (sizeof((x)->meta[0].chunk_len))

// size in bytes
#define SLOT_CHUNK_MEM_SIZE(x) (sizeof(**(x)->chunks))

// byte stride of an individual item; usually MAX(sizeof(t), 8)
#define SLOT_STRIDE(x) (sizeof((x)->chunks[0]->data[0]))


#define SLOT_ITEMP(x, c, i) (&((x)->chunks[c]->data[i].t))
#define SLOT_NEXTFREE(x, c, i) ((x)->chunks[c]->data[i].nextFree)

#define SLOT_OCC_BYTE(i) ((i) / 64)
#define SLOT_OCC_BIT(i) (1ul << ((i) % 64))
#define SLOT_GET_OCC(x, c, i) ((x)->chunks[(c)]->occupancy[SLOT_OCC_BYTE(i)] & SLOT_OCC_BIT(i))
#define SLOT_SET_OCC(x, c, i) ((x)->chunks[(c)]->occupancy[SLOT_OCC_BYTE(i)] |= SLOT_OCC_BIT(i))
#define SLOT_CLEAR_OCC(x, c, i) ((x)->chunks[(c)]->occupancy[SLOT_OCC_BYTE(i)] &= (~SLOT_OCC_BIT(i)))


// External API

// A pointer to the item at a specific index
// USER BEWARE: no regard to allocated length or index occupancy 
#define SLOT_index(x, i) SLOT_ITEMP((x), (i) / SLOT_CHUNK_LEN(x), (i) % SLOT_CHUNK_LEN(x))


// returns the slot index, given a pointer
#define SLOT_get_index(x, p) ({ \
	uint64_t __SLOT_c = 0, __SLOT_i = ULONG_MAX; \
	for(; __SLOT_c < (x)->b.chunksLen; __SLOT_c++) { \
		if(  \
			(void*)p >= (void*)((x)->chunks[__SLOT_c]->data) &&  \
			(void*)p <= (void*)((x)->chunks[__SLOT_c]->data) + sizeof((x)->chunks[__SLOT_c]->data) \
		) { \
			__SLOT_i = ((void*)p - (void*)(x)->chunks[__SLOT_c]->data) / SLOT_STRIDE(x); \
			break; \
		} \
	} \
	\
	(__SLOT_i == ULONG_MAX) ? ULONG_MAX : (__SLOT_i + __SLOT_c * (x)->b.chunksLen); \
})


// How many items are currently in the structure
#define SLOT_fill(x) ((x)->b.fill)


#define SLOT_inc(...) SLOT_inc_n(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define SLOT_inc_n(n, ...) CAT(SLOT_inc_, n)(__VA_ARGS__)

// just get a new slot and return the pointer
#define SLOT_inc_1(x) \
({ \
	if((x)->b.fill >= (x)->b.alloc) { \
		slot_resize(&(x)->b, SLOT_CHUNK_MEM_SIZE(x), SLOT_CHUNK_LEN(x)); \
	} \
	\
	uint64_t __SLOT_c, __SLOT_i; \
	uint64_t __SLOT_tmp = (x)->b.nextFree; \
	if(__SLOT_tmp == ULONG_MAX) { \
		/* fresh slot; the next one based on fill, which also works for an empty structure */ \
		__SLOT_c = SLOT_fill(x) / SLOT_CHUNK_LEN(x); \
		__SLOT_i = SLOT_fill(x) % SLOT_CHUNK_LEN(x); \
		\
	} \
	else { \
		__SLOT_c = __SLOT_tmp / SLOT_CHUNK_LEN(x); \
		__SLOT_i = __SLOT_tmp % SLOT_CHUNK_LEN(x); \
		\
		(x)->b.nextFree = SLOT_NEXTFREE(x, __SLOT_c, __SLOT_i); \
		\
		memset(SLOT_ITEMP(x, __SLOT_c, __SLOT_i), 0, SLOT_ITEM_SIZE(x)); \
	} \
	\
	SLOT_fill(x)++; \
	SLOT_SET_OCC(x, __SLOT_c, __SLOT_i); \
	SLOT_ITEMP(x, __SLOT_c, __SLOT_i); \
})


// on zero-init, fill = 0 and nextFree = 0, which is incidently correct:
//   if(fill == 0), set nextfree to the subsequent cell, and fill it 

// when nextFree == ULONG_MAX, the next cell is fill + 1:
// inserts: increment fill, leave nextFree unchanged
// deletes: set nextFree to the deleted index, set cell->nextFree to ULONG_MAX (will naturally restore at the end of the chain)

// upon first insert of zero-init, slot_resize() should set nextFree to ULONG_MAX
// 



// same thing, but also sets *'indexp' to the index of the new item
#define SLOT_inc_2(x, indexp) \
({ \
	if((x)->b.fill >= (x)->b.alloc) { \
		slot_resize(&(x)->b, SLOT_CHUNK_MEM_SIZE(x), SLOT_CHUNK_LEN(x)); \
	} \
	\
	u64 __SLOT_c, __SLOT_i; \
	u64 __SLOT_tmp = (x)->b.nextFree; \
	if(__SLOT_tmp == ULONG_MAX) { \
		/* fresh slot; the next one based on fill, which also works for an empty structure */ \
		*(indexp) = SLOT_fill(x); \
		__SLOT_c = SLOT_fill(x) / SLOT_CHUNK_LEN(x); \
		__SLOT_i = SLOT_fill(x) % SLOT_CHUNK_LEN(x); \
		\
	} \
	else { \
		*(indexp) = __SLOT_tmp; \
		__SLOT_c = __SLOT_tmp / SLOT_CHUNK_LEN(x); \
		__SLOT_i = __SLOT_tmp % SLOT_CHUNK_LEN(x); \
		\
		(x)->b.nextFree = SLOT_NEXTFREE(x, __SLOT_c, __SLOT_i); \
		\
		memset(SLOT_ITEMP(x, __SLOT_c, __SLOT_i), 0, SLOT_ITEM_SIZE(x)); \
	} \
	\
	SLOT_fill(x)++; \
	SLOT_SET_OCC(x, __SLOT_c, __SLOT_i); \
	SLOT_ITEMP(x, __SLOT_c, __SLOT_i); \
})




#define SLOT_push(x, o) \
({ \
	*SLOT_inc(x) = (o); \
})



// sets a specific index as occupied and returns a pointer.
// WARNING: completely breaks the freelist. Use with care.
#define SLOT_assert_index(x, i) \
({ \
	if(i >= (x)->b.alloc) { \
		slot_resize_to(&(x)->b, SLOT_CHUNK_MEM_SIZE(x), SLOT_CHUNK_LEN(x), i); \
	} \
	\
	uint64_t __SLOT_c, __SLOT_i; \
	__SLOT_c = (i) / SLOT_CHUNK_LEN(x); \
	__SLOT_i = (i) % SLOT_CHUNK_LEN(x); \
	\
	SLOT_fill(x)++; \
	SLOT_SET_OCC(x, __SLOT_c, __SLOT_i); \
	\
	SLOT_ITEMP(x, __SLOT_c, __SLOT_i); \
})


// reconstructs the freelist from scratch based on occupancy flags. mainly for use after SLOT_assert_index().
#define SLOT_rebuild_freelist(x) do { \
	\
	(x)->b.nextFree = ULONG_MAX; \
	\
	for(uint64_t __SLOT_c = 0; __SLOT_c < (x)->b.chunksLen; __SLOT_c++) { \
		for(uint64_t __SLOT_i = 0; __SLOT_i < SLOT_CHUNK_LEN(x); __SLOT_i++) { \
			\
			if(!SLOT_GET_OCC(x, __SLOT_c, __SLOT_i)) { \
				SLOT_NEXTFREE(x, __SLOT_c, __SLOT_i) = (x)->b.nextFree; \
				(x)->b.nextFree = __SLOT_i + __SLOT_c * SLOT_CHUNK_LEN(x); \
			} \
			\
		} \
	} \
} while(0)



#define SLOT_rm(x, p) \
do { \
	uint64_t __SLOT_c = 0, __SLOT_i = ULONG_MAX; \
	for(; __SLOT_c < (x)->b.chunksLen; __SLOT_c++) { \
		if(  \
			(void*)p >= (void*)((x)->chunks[__SLOT_c]->data) &&  \
			(void*)p <= (void*)((x)->chunks[__SLOT_c]->data) + sizeof((x)->chunks[__SLOT_c]->data) \
		) { \
			__SLOT_i = ((void*)p - (void*)(x)->chunks[__SLOT_c]->data) / SLOT_STRIDE(x); \
			break; \
		} \
	} \
	\
	if(!SLOT_GET_OCC(x, __SLOT_c, __SLOT_i)) { \
		/* dbg("slot not occupied: %ld:%ld (double free?)", __SLOT_c, __SLOT_i) */ \
		/* assert(0); */ \
		break; \
	} \
	\
	SLOT_NEXTFREE(x, __SLOT_c, __SLOT_i) = (x)->b.nextFree; \
	(x)->b.nextFree = __SLOT_i + __SLOT_c * SLOT_CHUNK_LEN(x); \
	\
	SLOT_CLEAR_OCC(x, __SLOT_c, __SLOT_i); \
	SLOT_fill(x)--; \
} while(0);



// doesn't complain about double frees
#define SLOT_rm_yolo(x, p) \
do { \
	uint64_t __SLOT_c = 0, __SLOT_i = ULONG_MAX; \
	for(; __SLOT_c < (x)->b.chunksLen; __SLOT_c++) { \
		if(  \
			(void*)p >= (void*)((x)->chunks[__SLOT_c]->data) &&  \
			(void*)p <= (void*)((x)->chunks[__SLOT_c]->data) + sizeof((x)->chunks[__SLOT_c]->data) \
		) { \
			__SLOT_i = ((void*)p - (void*)(x)->chunks[__SLOT_c]->data) / SLOT_STRIDE(x); \
			break; \
		} \
	} \
	\
	if(__SLOT_i == ULONG_MAX) break; \
	if(!SLOT_GET_OCC(x, __SLOT_c, __SLOT_i)) { \
		break; \
	} \
	\
	SLOT_NEXTFREE(x, __SLOT_c, __SLOT_i) = (x)->b.nextFree; \
	(x)->b.nextFree = __SLOT_i + __SLOT_c * SLOT_CHUNK_LEN(x); \
	\
	SLOT_CLEAR_OCC(x, __SLOT_c, __SLOT_i); \
	SLOT_fill(x)--; \
} while(0);



#define SLOT_rm_index(x, ind) \
do { \
	uint64_t __SLOT_c = (ind) / SLOT_CHUNK_LEN(x); \
	uint64_t __SLOT_i = (ind) % SLOT_CHUNK_LEN(x); \
	\
	SLOT_NEXTFREE(x, __SLOT_c, __SLOT_i) = (x)->b.nextFree; \
	(x)->b.nextFree = __SLOT_i + __SLOT_c * SLOT_CHUNK_LEN(x); \
	\
	SLOT_CLEAR_OCC(x, __SLOT_c, __SLOT_i); \
	SLOT_fill(x)--; \
} while(0);







#define SLOT_free(x) slot_free(&(x)->b); 



#define SLOT_EACH _Pragma("GCC error \"'SLOT_EACH' does not exist. Use 'SLOT_EACHP'.\"")


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
		finished: ; // used to exit the loop (break, failed condition)
	else
		for(char* keyname;;) // internal declarations, multiple loops to avoid comma op funny business
		for(valtype valname;;)
		for(void* iter = NULL ;;)
			if(HT_next(obj, iter, &keyname, &valname)) // first iteration loop condition
				goto main_loop;
			else
				while(1) // catches "break"
					if(1) {
						// when the user uses break
						goto finished;
					}
					else
						while(1) // operates the looping behavior
							if(!HT_next(obj, iter, &keyname, &valname)) { // second loop condition and later
								// normal termination
								goto finished;
							}
							else
								main_loop:
								//	{ user block; not in macro }
*/
// pointer version



// "index" is a sequential counter, unrelated to the slot index
#define SLOT__PASTE(a, b) CAT(a, b) 
#define SLOT__ITER_I(key, val) SLOT__PASTE(SLOT_iteri_ ## key ## __ ## val ## __, __LINE__)
#define SLOT__ITER_C(key, val) SLOT__PASTE(SLOT_iterc_ ## key ## __ ## val ## __, __LINE__)
#define SLOT__FINISHED(key, val) SLOT__PASTE(SLOT_finished__ ## key ## __ ## val ## __, __LINE__)
#define SLOT__MAINLOOP(key, val) SLOT__PASTE(SLOT_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define SLOT_EACHP(obj, index, valname) \
if(0) \
	SLOT__FINISHED(index, val): ; \
else \
	for(typeof((obj)->meta[0].tp) valname ;;) \
	for(size_t index = 0;;) \
	for(size_t SLOT__ITER_C(index, val) = 0;;) \
	for(size_t SLOT__ITER_I(index, val) = 0;;) \
		if( (obj)->b.fill > 0 \
			&& slot_next(&(obj)->b, SLOT_CHUNK_LEN(obj), &SLOT__ITER_C(index, val), &SLOT__ITER_I(index, val), 0) \
			&& (valname = SLOT_ITEMP(obj, SLOT__ITER_C(index, val), SLOT__ITER_I(index, val)), 1) \
		) \
			goto SLOT__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto SLOT__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if( \
							!slot_next(&(obj)->b, SLOT_CHUNK_LEN(obj), &SLOT__ITER_C(index, val), &SLOT__ITER_I(index, val), 1) \
							|| (valname = SLOT_ITEMP(obj, SLOT__ITER_C(index, val), SLOT__ITER_I(index, val)), 0) \
							|| (index++, 0) \
						) { \
							goto SLOT__FINISHED(index, val); \
						} \
						else \
							SLOT__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }



// "index" is the index into the slot
#define SLOT_EACHP_INDEX(obj, index, valname) \
if(0) \
	SLOT__FINISHED(index, val): ; \
else \
	for(typeof((obj)->meta[0].tp) valname ;;) \
	for(size_t index = 0;;) \
	for(size_t SLOT__ITER_C(index, val) = 0;;) \
	for(size_t SLOT__ITER_I(index, val) = 0;;) \
		if( (obj)->b.fill > 0 \
			&& slot_next(&(obj)->b, SLOT_CHUNK_LEN(obj), &SLOT__ITER_C(index, val), &SLOT__ITER_I(index, val), 0) \
			&& (valname = SLOT_ITEMP(obj, SLOT__ITER_C(index, val), SLOT__ITER_I(index, val)), 1) \
			&& (index = SLOT__ITER_C(index, val) * SLOT_CHUNK_LEN(obj) + SLOT__ITER_I(index, val), 1) \
		) \
			goto SLOT__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto SLOT__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if( \
							!slot_next(&(obj)->b, SLOT_CHUNK_LEN(obj), &SLOT__ITER_C(index, val), &SLOT__ITER_I(index, val), 1) \
							|| (valname = SLOT_ITEMP(obj, SLOT__ITER_C(index, val), SLOT__ITER_I(index, val)), 0) \
							|| (index = SLOT__ITER_C(index, val) * SLOT_CHUNK_LEN(obj) + SLOT__ITER_I(index, val), 0) \
						) { \
							goto SLOT__FINISHED(index, val); \
						} \
						else \
							SLOT__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }



// "index" is the index into the slot
#define SLOT_EACH_INDEX(obj, index, valname) \
if(0) \
	SLOT__FINISHED(index, val): ; \
else \
	for(typeof((obj)->meta[0].t) valname ;;) \
	for(size_t index = 0;;) \
	for(size_t SLOT__ITER_C(index, val) = 0;;) \
	for(size_t SLOT__ITER_I(index, val) = 0;;) \
		if( (obj)->b.fill > 0 \
			&& slot_next(&(obj)->b, SLOT_CHUNK_LEN(obj), &SLOT__ITER_C(index, val), &SLOT__ITER_I(index, val), 0) \
			&& (valname = *SLOT_ITEMP(obj, SLOT__ITER_C(index, val), SLOT__ITER_I(index, val)), 1) \
			&& (index = SLOT__ITER_C(index, val) * SLOT_CHUNK_LEN(obj) + SLOT__ITER_I(index, val), 1) \
		) \
			goto SLOT__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto SLOT__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if( \
							!slot_next(&(obj)->b, SLOT_CHUNK_LEN(obj), &SLOT__ITER_C(index, val), &SLOT__ITER_I(index, val), 1) \
							|| (valname = *SLOT_ITEMP(obj, SLOT__ITER_C(index, val), SLOT__ITER_I(index, val)), 0) \
							|| (index = SLOT__ITER_C(index, val) * SLOT_CHUNK_LEN(obj) + SLOT__ITER_I(index, val), 0) \
						) { \
							goto SLOT__FINISHED(index, val); \
						} \
						else \
							SLOT__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }





#endif // __sti__slot_h__
