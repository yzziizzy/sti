#ifndef __sti__vec_h__
#define __sti__vec_h__

// Public Domain.


#include <stddef.h> // size_t
#include <string.h> // memcpy


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
#define VEC_INIT(x) \
do { \
	(x)->data = NULL; \
	(x)->len = 0; \
	(x)->alloc = 0; \
} while(0)


// helpers
#define VEC_LEN(x) ((x)->len)
#define VEC_ALLOC(x) ((x)->alloc)
#define VEC_DATA(x) ((x)->data)
#define VEC_ITEM(x, i) (VEC_DATA(x)[(i)])

#define VEC_TAIL(x) (VEC_DATA(x)[VEC_LEN(x)-1])
#define VEC_HEAD(x) (VEC_DATA(x)[0])

#define VEC_FIND(x, ptr_o) vec_find(VEC_DATA(x), VEC_LEN(x), sizeof(*VEC_DATA(x)), ptr_o)

#define VEC_TRUNC(x) (VEC_LEN(x) = 0)
//  

#define VEC_GROW(x) vec_resize((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)))

// check if a size increase is needed to insert one more item
#define VEC_CHECK(x) \
do { \
	if(VEC_LEN(x) >= VEC_ALLOC(x)) { \
		VEC_GROW(x); \
	} \
} while(0)


// operations

// increase size and assign the new entry
#define VEC_PUSH(x, e) \
do { \
	VEC_CHECK(x); \
	VEC_DATA(x)[VEC_LEN(x)] = (e); \
	VEC_LEN(x)++; \
} while(0)

// increase size and evaluates to a pointer to it
#define VEC_INC(x) \
({ \
	VEC_CHECK(x); \
	VEC_LEN(x)++; \
	&VEC_TAIL(x); \
})


// set the size and zero the contents
#define VEC_CALLOC(x, sz) \
do { \
	if(VEC_ALLOC(x) < (sz)) { \
		vec_resize_to((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)), (sz)); \
	} \
	VEC_LEN(x) = (sz); \
	memset(VEC_DATA(x), 0, sizeof(*VEC_DATA(x)) * (sz)); \
} while(0)

// Ensure that the vec is at least of a certain size, and zero any newly allocated portion
#define VEC_CREALLOC(x, sz) \
do { \
	if(VEC_ALLOC(x) < (sz)) { \
		vec_c_resize_to((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)), (sz)); \
		VEC_LEN(x) = (sz); \
	} \
} while(0)




#define VEC_PREPEND(x, e) \
do { \
	VEC_CHECK(x); \
	memmove( \
		VEC_DATA(x) + 1, \
		VEC_DATA(x), \
		VEC_LEN(x) * sizeof(*VEC_DATA(x)) \
	); \
	VEC_DATA(x)[0] = (e); \
	VEC_LEN(x)++; \
} while(0)


#define VEC_PEEK(x) VEC_DATA(x)[VEC_LEN(x) - 1]

#define VEC_POP(x, e) \
do { \
	if(VEC_LEN(x) > 0) { \
		(e) = VEC_PEEK(x); \
		VEC_LEN(x)--; \
	} \
} while(0)

#define VEC_POP1(x) \
do { \
	if(VEC_LEN(x) > 0) { \
		VEC_LEN(x)--; \
	} \
} while(0)


// ruins order but is O(1). meh.
#define VEC_RM(x, i) \
do { \
	if(VEC_LEN(x) < (i)) break; \
	VEC_ITEM(x, i) = VEC_PEEK(x); \
	VEC_LEN(x)--; \
} while(0)

// preserves order. O(n)
#define VEC_RM_SAFE(x, i) \
do { \
	if(VEC_LEN(x) < (i)) break; \
	memmove( \
		(char*)VEC_DATA(x) + ((i) * sizeof(*VEC_DATA(x))), \
		(char*)VEC_DATA(x) + (((i) + 1) * sizeof(*VEC_DATA(x))), \
		(VEC_LEN(x) - (i)) * sizeof(*VEC_DATA(x)) \
	); \
	VEC_LEN(x)--; \
} while(0)



// ruins order but is O(1). meh.
#define VEC_RM_VAL(x, ptr_o) vec_rm_val((char*)VEC_DATA(x), &VEC_LEN(x), sizeof(*VEC_DATA(x)), ptr_o)



// TODO: vec_set_ins // sorted insert
// TODO: vec_set_rem
// TODO: vec_set_has

// TODO: vec_purge // search and delete of all entries

#define VEC_FREE(x) \
do { \
	if(VEC_DATA(x)) free(VEC_DATA(x)); \
	VEC_DATA(x) = NULL; \
	VEC_LEN(x) = 0; \
	VEC_ALLOC(x) = 0; \
} while(0)

#define VEC_COPY(dst, src) vec_copy( \
	(char**)&VEC_DATA(dst), (char*)VEC_DATA(src), \
	&VEC_ALLOC(dst), VEC_ALLOC(src), \
	&VEC_LEN(dst), VEC_LEN(src), \
	sizeof(*VEC_DATA(src)))

#define VEC_REVERSE(x) \
do { \
	size_t i, j; \
	void* tmp = alloca(sizeof(*VEC_DATA(x))); \
	for(i = 0, j = VEC_LEN(x); i < j; i++, j--) { \
		memcpy(tmp, VEC_DATA(x)[i]); \
		memcpy(VEC_DATA(x)[i], VEC_DATA(x)[j]); \
		memcpy(VEC_DATA(x)[j], tmp); \
	} \
} while(0)



#define VEC_INSERT_AT(x, e, i) \
do { \
	VEC_CHECK(x); \
	memmove( /* move the rest of x forward */ \
		VEC_DATA(x) + (i) + 1, \
		VEC_DATA(x) + (i),  \
		(VEC_LEN(x) - (i)) * sizeof(*VEC_DATA(x)) \
	); \
	VEC_DATA(x)[i] = (e); \
} while(0)


#define VEC_SPLICE(x, y, where) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(x) + VEC_LEN(y)) { \
		vec_resize_to((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)), VEC_LEN(x) + VEC_LEN(y)); \
	} \
	\
	memmove( /* move the rest of x forward */ \
		VEC_DATA(x) + ((where) + VEC_LEN(y)), \
		VEC_DATA(x) + (where),  \
		(VEC_LEN(x) - (where)) * sizeof(*VEC_DATA(x)) \
	); \
	memcpy( /* copy y into the space created */ \
		VEC_DATA(x) + (where), \
		VEC_DATA(y),  \
		VEC_LEN(y) * sizeof(*VEC_DATA(y)) \
	); \
	VEC_LEN(x) += VEC_LEN(y); \
} while(0)


// concatenate y onto the end of x
#define VEC_CAT(x, y) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(x) + VEC_LEN(y)) { \
		vec_resize_to((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)), VEC_LEN(x) + VEC_LEN(y)); \
	} \
	memcpy( \
		VEC_DATA(x) + VEC_LEN(x), \
		VEC_DATA(y),  \
		VEC_LEN(y) * sizeof(*VEC_DATA(y)) \
	); \
	VEC_LEN(x) += VEC_LEN(y); \
} while(0)


// make some space somewhere
#define VEC_RESERVE(x, len, where) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(x) + (len)) { \
		vec_resize_to((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)), VEC_LEN(x) + (len)); \
	} \
	\
	memmove( /* move the rest of x forward */ \
		VEC_DATA(x) + (where) + (len), \
		VEC_DATA(x) + (where),  \
		(VEC_LEN(x) - (where)) * sizeof(*VEC_DATA(x)) \
	); \
	VEC_LEN(x) += (len); \
} while(0)


// copy data from y into x at where, overwriting existing data in x
// extends x if it would overlap the end
#define VEC_OVERWRITE(x, y, where) \
do { \
	if(VEC_ALLOC(x) < VEC_LEN(y) + (where)) { \
		vec_resize_to((void**)&VEC_DATA(x), &VEC_ALLOC(x), sizeof(*VEC_DATA(x)), where + VEC_LEN(y)); \
	} \
	memcpy( /* copy y into the space created */ \
		VEC_DATA(x) + where, \
		VEC_DATA(y),  \
		VEC_LEN(y) * sizeof(*VEC_DATA(y)) \
	); \
	VEC_LEN(x) = MAX(VEC_LEN(x), VEC_LEN(y) + (where)); \
} while(0)



#define VEC_SORT(x, fn) \
	qsort(VEC_DATA(x), VEC_LEN(x), sizeof(*VEC_DATA(x)), (int(*)(const void*,const void*))fn);

#define VEC_SORT_R(x, fn, s) \
	qsort_r(VEC_DATA(x), VEC_LEN(x), sizeof(*VEC_DATA(x)), (int(*)(const void*,const void*,void*))fn, (void*)s);


// moves the specified index into sorted position inside an otherwise sorted vec
// IMPORTANT: the vec is assumed to be sorted except the specified index
// returns the new index
#define VEC_BUBBLE_INDEX(x, i, fn) \
	vec_bubble_index(VEC_DATA(x), VEC_LEN(x), sizeof(*VEC_DATA(x)), (i), (int(*)(const void*,const void*))fn);




#define VEC_UNIQ(x, fn) \
	vec_uniq(VEC_DATA(x), &VEC_LEN(x), sizeof(*VEC_DATA(x)), (int(*)(const void*,const void*))fn);

#define VEC_UNIQ_R(x, fn) \
	vec_uniq_r(VEC_DATA(x), &VEC_LEN(x), sizeof(*VEC_DATA(x)), (int(*)(const void*,const void*))fn);



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
	for(typeof(*VEC_DATA(obj)) valname ;;) \
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
	for(typeof(*VEC_DATA(obj))* valname ;;) \
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
	for(typeof(*VEC_DATA(obj)) valname ;;) \
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
