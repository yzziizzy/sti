#ifndef __sti__vec_h__
#define __sti__vec_h__

// Public Domain.


#include <stddef.h> // size_t
#include <sys/types.h> // ssize_t
#include <string.h> // memcpy

#include "macros.h"

// -----------------------
// non-thread-safe vectors
// -----------------------

// declare a vector
#define VEC(t) \
struct { \
	size_t len, alloc; \
	t* data; \
}

// initialisze a vector
#define VEC_init(x) \
do { \
	(x)->data = NULL; \
	(x)->len = 0; \
	(x)->alloc = 0; \
} while(0)

#define VEC_INIT(x) VEC_init(x) 

// helpers
#define VEC_LEN(x) ((x)->len)
#define VEC_len(x) ((x)->len)
#define VEC_ALLOC(x) ((x)->alloc)
#define VEC_alloc(x) ((x)->alloc)
#define VEC_DATA(x) ((x)->data)
#define VEC_data(x) ((x)->data)
#define VEC_ITEM(x, i) (VEC_data(x)[(i)])
#define VEC_item(x, i) (VEC_data(x)[(i)])


#define VEC_TAIL(x) (VEC_data(x)[VEC_LEN(x)-1])
#define VEC_tail(x) (VEC_data(x)[VEC_LEN(x)-1])
#define VEC_HEAD(x) (VEC_data(x)[0])
#define VEC_head(x) (VEC_data(x)[0])

#define VEC_FIND(x, ptr_o) vec_find(VEC_data(x), VEC_LEN(x), sizeof(*VEC_data(x)), ptr_o)
#define VEC_find(x, ptr_o) vec_find(VEC_data(x), VEC_LEN(x), sizeof(*VEC_data(x)), ptr_o)

#define VEC_TRUNC(x) (VEC_LEN(x) = 0)
#define VEC_trunc(x) (VEC_LEN(x) = 0)
//  

#define VEC_GROW(x) vec_resize((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)))

// check if a size increase is needed to insert one more item
#define VEC_CHECK(x) \
do { \
	if(VEC_LEN(x) >= VEC_ALLOC(x)) { \
		VEC_GROW(x); \
	} \
} while(0)


// operations

// increase size and assign the new entry
#define VEC_PUSH(x, ...) VEC_push(x, __VA_ARGS__)
#define VEC_push(x, ...) \
do { \
	VEC_CHECK(x); \
	VEC_data(x)[VEC_LEN(x)] = (__VA_ARGS__); \
	VEC_LEN(x)++; \
} while(0)

// increase size and evaluates to a pointer to it
#define VEC_INC(x) VEC_inc(x)
#define VEC_inc(x) \
({ \
	VEC_CHECK(x); \
	VEC_LEN(x)++; \
	memset(&VEC_TAIL(x), 0, sizeof(*VEC_data(x))); \
	&VEC_TAIL(x); \
})

// set the size and zero the contents
#define VEC_CALLOC(x, sz) VEC_calloc(x, sz)
#define VEC_calloc(x, sz) \
do { \
	if(VEC_ALLOC(x) < (sz)) { \
		vec_resize_to((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)), (sz)); \
	} \
	VEC_LEN(x) = (sz); \
	memset(VEC_data(x), 0, sizeof(*VEC_data(x)) * (sz)); \
} while(0)

// Ensure that the vec is at least of a certain size, and zero any newly allocated portion
#define VEC_CREALLOC(x, sz) VEC_crealloc(x, sz)
#define VEC_crealloc(x, sz) \
do { \
	if(VEC_ALLOC(x) < (sz)) { \
		vec_c_resize_to((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)), (sz)); \
		VEC_LEN(x) = (sz); \
	} \
} while(0)




#define VEC_PREPEND(x, e) VEC_prepend(x, e)
#define VEC_prepend(x, e) \
do { \
	VEC_CHECK(x); \
	memmove( \
		VEC_data(x) + 1, \
		VEC_data(x), \
		VEC_LEN(x) * sizeof(*VEC_data(x)) \
	); \
	VEC_data(x)[0] = (e); \
	VEC_LEN(x)++; \
} while(0)


#define VEC_PEEK(x) VEC_peek(x)
#define VEC_peek(x) VEC_data(x)[VEC_LEN(x) - 1]

#define VEC_POP(x, e) VEC_pop(x, e)
#define VEC_pop(...) VEC_pop_N(PP_NARG(__VA_ARGS__), __VA_ARGS__) 
#define VEC_pop_N(n, ...) CAT(VEC_pop_, n)(__VA_ARGS__)
#define VEC_pop_2(x, e) \
do { \
	if(VEC_LEN(x) > 0) { \
		(e) = VEC_PEEK(x); \
		VEC_LEN(x)--; \
	} \
} while(0)

#define VEC_POP1(x) VEC_pop_1(x)
#define VEC_pop_1(x) \
({ \
	if(VEC_LEN(x) > 0) { \
		VEC_LEN(x)--; \
	} \
	VEC_item(x, VEC_len(x)); \
})


// ruins order but is O(1). meh.
#define VEC_RM(x, i) VEC_rm(x, i)
#define VEC_rm(x, i) \
do { \
	if(VEC_LEN(x) < (i)) break; \
	VEC_ITEM(x, i) = VEC_PEEK(x); \
	VEC_LEN(x)--; \
} while(0)

// preserves order. O(n)
#define VEC_RM_SAFE(x, i) VEC_rm_safe(x, i)
#define VEC_rm_safe(x, i) \
do { \
	if(VEC_LEN(x) < (i)) break; \
	memmove( \
		(char*)VEC_data(x) + ((i) * sizeof(*VEC_data(x))), \
		(char*)VEC_data(x) + (((i) + 1) * sizeof(*VEC_data(x))), \
		(VEC_LEN(x) - (i)) * sizeof(*VEC_data(x)) \
	); \
	VEC_LEN(x)--; \
} while(0)



// ruins order but is O(1). meh. TODO: add type check on the val
#define VEC_rm_val(x, ptr_o) vec_rm_val((char*)VEC_data(x), &VEC_LEN(x), sizeof(*VEC_data(x)), ptr_o)
#define VEC_RM_VAL(x, ptr_o) vec_rm_val((char*)VEC_data(x), &VEC_LEN(x), sizeof(*VEC_data(x)), ptr_o)



// TODO: vec_set_ins // sorted insert
// TODO: vec_set_rem
// TODO: vec_set_has

// TODO: vec_purge // search and delete of all entries

#define VEC_FREE(x) VEC_free(x)
#define VEC_free(x) \
do { \
	if(VEC_data(x)) free(VEC_data(x)); \
	VEC_data(x) = NULL; \
	VEC_LEN(x) = 0; \
	VEC_ALLOC(x) = 0; \
} while(0)

#define VEC_COPY(dst, src) VEC_copy(dst, src)
#define VEC_copy(dst, src) vec_copy( \
	(char**)&VEC_data(dst), (char*)VEC_data(src), \
	&VEC_ALLOC(dst), VEC_ALLOC(src), \
	&VEC_LEN(dst), VEC_LEN(src), \
	sizeof(*VEC_data(src)))

#define VEC_REVERSE(x) VEC_reverse(x)
#define VEC_reverse(x) \
do { \
	size_t i, j; \
	void* tmp = alloca(sizeof(*VEC_data(x))); \
	for(i = 0, j = VEC_LEN(x); i < j; i++, j--) { \
		memcpy(tmp, VEC_data(x)[i]); \
		memcpy(VEC_data(x)[i], VEC_data(x)[j]); \
		memcpy(VEC_data(x)[j], tmp); \
	} \
} while(0)



#define VEC_INSERT_AT(x, e, i) VEC_insert_at(x, e, i)
#define VEC_insert_at(x, e, i) \
do { \
	VEC_CHECK(x); \
	memmove( /* move the rest of x forward */ \
		VEC_data(x) + (i) + 1, \
		VEC_data(x) + (i),  \
		(VEC_LEN(x) - (i)) * sizeof(*VEC_data(x)) \
	); \
	VEC_data(x)[i] = (e); \
} while(0)


#define VEC_SPLICE(x, y, where) VEC_splice(x, y, where)
#define VEC_splice(x, y, where) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(x) + VEC_LEN(y)) { \
		vec_resize_to((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)), VEC_LEN(x) + VEC_LEN(y)); \
	} \
	\
	memmove( /* move the rest of x forward */ \
		VEC_data(x) + ((where) + VEC_LEN(y)), \
		VEC_data(x) + (where),  \
		(VEC_LEN(x) - (where)) * sizeof(*VEC_data(x)) \
	); \
	memcpy( /* copy y into the space created */ \
		VEC_data(x) + (where), \
		VEC_data(y),  \
		VEC_LEN(y) * sizeof(*VEC_data(y)) \
	); \
	VEC_LEN(x) += VEC_LEN(y); \
} while(0)


// concatenate y onto the end of x
#define VEC_CAT(x, y) VEC_cat(x,y)
#define VEC_cat(x, y) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(x) + VEC_LEN(y)) { \
		vec_resize_to((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)), VEC_LEN(x) + VEC_LEN(y)); \
	} \
	memcpy( \
		VEC_data(x) + VEC_LEN(x), \
		VEC_data(y),  \
		VEC_LEN(y) * sizeof(*VEC_data(y)) \
	); \
	VEC_LEN(x) += VEC_LEN(y); \
} while(0)


// make some space somewhere
#define VEC_RESERVE(x, len, where) VEC_reserve(x,len,where)
#define VEC_reserve(x, len, where) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(x) + (len)) { \
		vec_resize_to((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)), VEC_LEN(x) + (len)); \
	} \
	\
	memmove( /* move the rest of x forward */ \
		VEC_data(x) + (where) + (len), \
		VEC_data(x) + (where),  \
		(VEC_LEN(x) - (where)) * sizeof(*VEC_data(x)) \
	); \
	VEC_LEN(x) += (len); \
} while(0)


// copy data from y into x at where, overwriting existing data in x
// extends x if it would overlap the end
#define VEC_OVERWRITE(x, y, where) VEC_overwrite(x,y,where)
#define VEC_overwrite(x, y, where) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(y) + (where)) { \
		vec_resize_to((void**)&VEC_data(x), &VEC_ALLOC(x), sizeof(*VEC_data(x)), where + VEC_LEN(y)); \
	} \
	memcpy( /* copy y into the space created */ \
		VEC_data(x) + where, \
		VEC_data(y),  \
		VEC_LEN(y) * sizeof(*VEC_data(y)) \
	); \
	VEC_LEN(x) = MAX(VEC_LEN(x), VEC_LEN(y) + (where)); \
} while(0)



#define VEC_SORT(x, fn) VEC_sort(x, fn)
#define VEC_sort(x, fn) \
	qsort(VEC_data(x), VEC_LEN(x), sizeof(*VEC_data(x)), (int(*)(const void*,const void*))fn);

#define VEC_SORT_R(x, fn, s) VEC_sort_r(x,fn,s)
#define VEC_sort_r(x, fn, s) \
	qsort_r(VEC_data(x), VEC_LEN(x), sizeof(*VEC_data(x)), (int(*)(const void*,const void*,void*))fn, (void*)s);


// moves the specified index into sorted position inside an otherwise sorted vec
// IMPORTANT: the vec is assumed to be sorted except the specified index
// returns the new index
#define VEC_BUBBLE_INDEX(x, i, fn) VEC_bubble_index(x,i,fn)
#define VEC_bubble_index(x, i, fn) \
	vec_bubble_index(VEC_data(x), VEC_LEN(x), sizeof(*VEC_data(x)), (i), (int(*)(const void*,const void*))fn);




#define VEC_UNIQ(x, fn) VEC_uniq(x,fn)
#define VEC_uniq(x, fn) \
	vec_uniq(VEC_data(x), &VEC_LEN(x), sizeof(*VEC_data(x)), (int(*)(const void*,const void*))fn);

#define VEC_UNIQ_R(x, fn) VEC_uniq_r(x, fn)
#define VEC_uniq_r(x, fn) \
	vec_uniq_r(VEC_data(x), &VEC_LEN(x), sizeof(*VEC_data(x)), (int(*)(const void*,const void*))fn);



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
#define VEC__PASTEINNER(a, b) a ## b
#define VEC__PASTE(a, b) VEC__PASTEINNER(a, b) 
#define VEC__ITER(key, val) VEC__PASTE(VEC_iter_ ## key ## __ ## val ## __, __LINE__)
#define VEC__FINISHED(key, val) VEC__PASTE(VEC_finished__ ## key ## __ ## val ## __, __LINE__)
#define VEC__MAINLOOP(key, val) VEC__PASTE(VEC_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define VEC_EACH(obj, index, valname) \
if(0) \
	VEC__FINISHED(index, val): ; \
else \
	for(typeof(*VEC_data(obj)) valname ;;) \
	for(size_t index = 0;;) \
		if(index < VEC_LEN(obj) && (valname = VEC_ITEM(obj, index), 1)) \
			goto VEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= VEC_LEN(obj) || (valname = VEC_ITEM(obj, index), 0)) { \
							goto VEC__FINISHED(index, val); \
						} \
						else \
							VEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }
							
							
// pointer version
#define VEC_EACHP(obj, index, valname) \
if(0) \
	VEC__FINISHED(index, val): ; \
else \
	for(typeof(*VEC_data(obj))* valname ;;) \
	for(size_t index = 0;;) \
		if(index < VEC_LEN(obj) && (valname = &VEC_ITEM(obj, index), 1)) \
			goto VEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= VEC_LEN(obj) || (valname = &VEC_ITEM(obj, index), 0)) { \
							goto VEC__FINISHED(index, val); \
						} \
						else \
							VEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }

// reverse
#define VEC_R_EACH(obj, index, valname) \
if(0) \
	VEC__FINISHED(index, val): ; \
else \
	for(typeof(*VEC_data(obj)) valname ;;) \
	for(ptrdiff_t index = (ptrdiff_t)VEC_LEN(obj) - 1;;) \
		if(index >= 0 && (valname = VEC_ITEM(obj, index), 1)) \
			goto VEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(--index < 0 || (valname = VEC_ITEM(obj, index), 0)) { \
							goto VEC__FINISHED(index, val); \
						} \
						else \
							VEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }

// reverse
#define VEC_R_EACHP(obj, index, valname) \
if(0) \
	VEC__FINISHED(index, val): ; \
else \
	for(typeof(*VEC_data(obj))* valname ;;) \
	for(ptrdiff_t index = (ptrdiff_t)VEC_LEN(obj) - 1;;) \
		if(index >= 0 && (valname = &VEC_ITEM(obj, index), 1)) \
			goto VEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(--index < 0 || (valname = &VEC_ITEM(obj, index), 0)) { \
							goto VEC__FINISHED(index, val); \
						} \
						else \
							VEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }



// this version only iterates the index   
#define VEC_LOOP(obj, index) \
if(0) \
	VEC__FINISHED(index, val): ; \
else \
	for(size_t index = 0;;) \
		if(index < VEC_LEN(obj)) \
			goto VEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= VEC_LEN(obj)) { \
							goto VEC__FINISHED(index, val); \
						} \
						else \
							VEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }

// reverse; this version only iterates the index   
#define VEC_R_LOOP(obj, index) \
if(0) \
	VEC__FINISHED(index, val): ; \
else \
	for(ptrdiff_t index = (ptrdiff_t)VEC_LEN(obj) - 1;;) \
		if(index >= 0) \
			goto VEC__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VEC__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(--index < 0) { \
							goto VEC__FINISHED(index, val); \
						} \
						else \
							VEC__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }






// Do not use these directly.
void vec_resize(void** data, size_t* size, size_t elem_size);
ptrdiff_t vec_find(void* data, size_t len, size_t stride, void* search);
ptrdiff_t vec_rm_val(char* data, size_t* len, size_t stride, void* search);
void vec_resize_to(void** data, size_t* size, size_t elem_size, size_t new_size);
void vec_c_resize_to(void** data, size_t* size, size_t elem_size, size_t new_size);
void vec_copy(
	char** dst_data, char* src_data, 
	size_t* dst_alloc, size_t src_alloc, 
	size_t* dst_len, size_t src_len, 
	size_t elem_size
);


ssize_t vec_bubble_index(void* data, size_t len, size_t stride, size_t index, int (*cmp)(const void*,const void*));

void vec_uniq(void* data, size_t* lenp, size_t stride, int (*cmp)(const void*,const void*));
void vec_uniq_r(void* data, size_t* lenp, size_t stride, int (*cmp)(const void*,const void*,void*), void* arg);



#endif // __sti__vec_h__
