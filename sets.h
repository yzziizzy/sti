#ifndef __sti__sets_h__
#define __sti__sets_h__

// Public Domain.


#include <stddef.h> // size_t, ptrdiff_t

#include <stdint.h> // only used for the declarations below, not used in implementation

/*********************************
      Pointer Sets
**********************************

Built around sorted vectors
Roughly, tsearch is faster over about 750 inserts. 


*/

typedef struct PointerSet {
	void** set;
	size_t length;
	size_t alloc;
} PointerSet;


void PointerSet_print(PointerSet* ps);
void PointerSet_insert(PointerSet* ps, void* p);
int PointerSet_remove(PointerSet* ps, void* p);
int PointerSet_exists(PointerSet* ps, void* p);
PointerSet* PointerSet_alloc(void);
void PointerSet_init(PointerSet* ps);
void PointerSet_free(PointerSet* ps);
void PointerSet_destroy(PointerSet* ps);
int PointerSet_equal(PointerSet* a, PointerSet* b);
PointerSet* PointerSet_intersect(PointerSet* a, PointerSet* b);
PointerSet* PointerSet_union(PointerSet* a, PointerSet* b);
PointerSet* PointerSet_difference(PointerSet* a, PointerSet* b);
size_t PointerSet_find_index(PointerSet* ps, void* p);

// adds b into a 
void PointerSet_union_inplace(PointerSet* a, PointerSet* b);




/*********************************
      Generic Struct Sets
**********************************


*/

typedef int (*SetCmpFn)(void*, void*);

typedef struct StructSet {
	void* set;
	size_t length;
	size_t alloc;
	size_t elem_size;
	SetCmpFn cmp;
} StructSet;


#define StructSet_init(ss, type, comp_fn) \
do { \
	(ss)->length = 0; \
	(ss)->alloc = 0; \
	(ss)->elem_size = sizeof(type); \
	(ss)->cmp = (SetCmpFn)(comp_fn); \
} while(0)



int StructSet_insert(StructSet* ss, void* p);
int StructSet_insertGet(StructSet* ss, void* p, void** existing);
int StructSet_remove(StructSet* ss, void* p);
int StructSet_exists(StructSet* ss, void* p);
#define StructSet_alloc(e, c) StructSet_alloc_(sizeof(e), cmp)
StructSet* StructSet_alloc_(size_t elem_size, SetCmpFn cmp);
void StructSet_free(StructSet* ss);
void StructSet_destroy(StructSet* ss);

// must be the exact same set types and compare functions
StructSet* StructSet_intersect(StructSet* a, StructSet* b); \
StructSet* StructSet_union(StructSet* a, StructSet* b); \
StructSet* StructSet_difference(StructSet* a, StructSet* b); \
size_t StructSet_find_index(StructSet* ss, void* p);



/*********************************
      Generic Base Typed Sets
**********************************


*/

#define DECLARE_SET_FOR_TYPE(type) \
	typedef struct type##Set { \
		type* set; \
		size_t length; \
		size_t alloc; \
	} type##Set; \
	\
	void type##Set_print(type##Set* ps); \
	void type##Set_insert(type##Set* ps, type p); \
	int type##Set_remove(type##Set* ps, type p); \
	int type##Set_exists(type##Set* ps, type p); \
	type##Set* type##Set_alloc(void); \
	void type##Set_init(type##Set* ps); \
	void type##Set_free(type##Set* ps); \
	void type##Set_destroy(type##Set* ps); \
	type##Set* type##Set_intersect(type##Set* a, type##Set* b); \
	type##Set* type##Set_union(type##Set* a, type##Set* b); \
	void type##Set_union_inplace(type##Set* a, type##Set* b); \
	type##Set* type##Set_difference(type##Set* a, type##Set* b); \
	size_t type##Set_find_index(type##Set* ps, type p);





#define DEFINE_SET_INSERT(type) \
void type##Set_insert(type##Set* ps, type p) { \
	\
	if(ps->length == 0) { \
		ps->alloc = 8;  \
		ps->set = calloc(1, ps->alloc * sizeof(*ps->set));  \
		  \
		ps->set[0] = p;  \
		ps->length++;  \
		return;  \
	}  \
	else if(ps->length + 1 > ps->alloc) {  \
		ps->alloc *= 2;  \
		ps->set = realloc(ps->set, ps->alloc * sizeof(*ps->set));  \
	}   \
	\
	size_t i = type##Set_find_index(ps, p);  \
	if(ps->set[i] == p) return;  \
  \
	memmove(ps->set + i + 1, ps->set + i, (ps->length - i) * sizeof(*ps->set));  \
	ps->set[i] = p;  \
	ps->length++;  \
}


#define DEFINE_SET_PRINT(type, fmt) \
void type##Set_print(type##Set* ps) { \
	printf(#type "Set %p (%ld items)\n", (void*)ps, ps->length); \
	for(size_t i = 0; i < ps->length; i++) { \
		printf(" %ld: " fmt "\n", i, ps->set[i]); \
	} \
}


#define DEFINE_SET_FIND_INDEX(type) \
size_t type##Set_find_index(type##Set* ps, type p) { \
	ptrdiff_t R = ps->length - 1; \
	ptrdiff_t L = 0; \
	ptrdiff_t i; \
	 \
	while(R - L > 0) { \
		 \
		i = L + ((R - L) / 2); \
		if(ps->set[i] < p) { \
			L = i + 1; \
		} \
		else if(ps->set[i] > p) { \
			R = i - 1; \
		} \
		else { \
			return i; \
		} \
	} \
	 \
	return (ps->set[L] < p) ? L + 1 : L; \
} 


#define DEFINE_SET_REMOVE(type) \
int type##Set_remove(type##Set* ps, type p) { \
	if(ps->length == 0) return 0; \
	 \
	size_t i = type##Set_find_index(ps, p); \
	if(ps->set[i] != p) return 0; \
	 \
	memmove(ps->set + i, ps->set + i + 1, (ps->length - i - 1) * sizeof(*ps->set)); \
	ps->length--; \
	return 1; \
}

#define DEFINE_SET_EXISTS(type) \
int type##Set_exists(type##Set* ps, type p) { \
	size_t i = type##Set_find_index(ps, p); \
	return (ps->set[i] == p); \
}

#define DEFINE_SET_ALLOC(type) \
type##Set* type##Set_alloc(void) { \
	type##Set* ps = malloc(sizeof(*ps)); \
	type##Set_init(ps); \
	return ps; \
}

#define DEFINE_SET_INIT(type) \
void type##Set_init(type##Set* ps) { \
	ps->alloc = 0; \
	ps->length = 0; \
}

#define DEFINE_SET_FREE(type) \
void type##Set_free(type##Set* ps) { \
	free(ps->set); \
	free(ps); \
}

#define DEFINE_SET_DESTROY(type) \
void type##Set_destroy(type##Set* ps) { \
	free(ps->set); \
}


#define DEFINE_SET_INTERSECT(type) \
type##Set* type##Set_intersect(type##Set* a, type##Set* b) { \
	type##Set* c = malloc(sizeof(*c)); \
	 \
	c->alloc = a->length > b->length ? a->length : b->length; \
	c->set = malloc(c->alloc * sizeof(*c->set)); \
	c->length = 0; \
	 \
	size_t ci = 0; \
	size_t bi = 0; \
	size_t ai = 0; \
	while(ai < a->length && bi < b->length) { \
		type ap = a->set[ai]; \
		type bp = b->set[bi]; \
		if(ap == bp) { \
			c->set[ci] = a->set[ai]; \
			c->length++; \
			ai++; bi++; ci++;  \
		} \
		else if(ap > bp) { \
			bi++; \
		} \
		else { \
			ai++; \
		} \
	} \
	 \
	return c; \
}

#define DEFINE_SET_UNION(type) \
type##Set* type##Set_union(type##Set* a, type##Set* b) { \
	type##Set* c = malloc(sizeof(*c)); \
	 \
	c->alloc = a->length + b->length; \
	c->set = malloc(c->alloc * sizeof(*c->set)); \
	c->length = 0; \
	 \
	size_t ci = 0; \
	size_t bi = 0; \
	size_t ai = 0; \
	while(ai < a->length || bi < b->length) { \
		type ap = a->set[ai]; \
		type bp = b->set[bi]; \
		if(ap == bp) { \
			c->set[ci] = a->set[ai]; \
			c->length++; \
			ai++; bi++; ci++;  \
		} \
		else if(ap > bp) { \
			c->set[ci++] = b->set[bi]; \
			c->length++; \
			bi++; \
		} \
		else { \
			c->set[ci++] = a->set[ai]; \
			c->length++; \
			ai++; \
		} \
	} \
	 \
	return c; \
}

#define DEFINE_SET_UNION_INPLACE(type) \
void type##Set_union_inplace(type##Set* a, type##Set* b) { \
	type##Set* c = malloc(sizeof(*c)); \
	\
	a->alloc = a->length + b->length; \
	a->set = realloc(a->set, a->alloc * sizeof(*a->set)); \
	\
	size_t final_length = 0; \
	ptrdiff_t bi = b->length - 1; \
	ptrdiff_t ai = a->length - 1; \
	ptrdiff_t wi = a->alloc - 1; \
	while(ai >= 0 && bi >= 0) { \
		type ap = a->set[ai]; \
		type bp = b->set[bi]; \
		if(ap == bp) { \
			a->set[wi] = a->set[ai]; \
			final_length++; \
			ai--; bi--; wi--;  \
		} \
		else if(ap < bp) { \
			a->set[wi--] = b->set[bi--]; \
			final_length++; \
		} \
		else { \
			a->set[wi--] = a->set[ai--]; \
			final_length++; \
		} \
	}; \
	 \
	while(ai >= 0) { \
		a->set[wi--] = a->set[ai--]; \
		final_length++; \
	} \
	while(bi >= 0) { \
		a->set[wi--] = b->set[bi--]; \
		final_length++; \
	} \
	 \
	memmove(a->set, a->set + a->alloc - final_length, final_length * sizeof(*a->set)); \
	a->length = final_length; \
}

#define DEFINE_SET_DIFFERENCE(type) \
type##Set* type##Set_difference(type##Set* a, type##Set* b) { \
	type##Set* c = malloc(sizeof(*c)); \
	 \
	c->alloc = a->length + b->length; \
	c->set = malloc(c->alloc * sizeof(*c->set)); \
	c->length = 0; \
	 \
	size_t ci = 0; \
	size_t bi = 0; \
	size_t ai = 0; \
	while(ai < a->length || bi < b->length) { \
		type ap = a->set[ai]; \
		type bp = b->set[bi]; \
		if(ap == bp) { \
			ai++; bi++;  \
		} \
		else if(ap > bp) { \
			c->set[ci++] = b->set[bi]; \
			c->length++; \
			bi++; \
		} \
		else { \
			c->set[ci++] = a->set[ai]; \
			c->length++; \
			ai++; \
		} \
	} \
	 \
	return c; \
}

#define DEFINE_SET_FOR_TYPE(type, fmt) \
	DEFINE_SET_DIFFERENCE(type) \
	DEFINE_SET_UNION(type) \
	DEFINE_SET_UNION_INPLACE(type) \
	DEFINE_SET_INTERSECT(type) \
	DEFINE_SET_ALLOC(type) \
	DEFINE_SET_INIT(type) \
	DEFINE_SET_FREE(type) \
	DEFINE_SET_DESTROY(type) \
	DEFINE_SET_INSERT(type) \
	DEFINE_SET_REMOVE(type) \
	DEFINE_SET_EXISTS(type) \
	DEFINE_SET_PRINT(type, fmt) \
	DEFINE_SET_FIND_INDEX(type)




DECLARE_SET_FOR_TYPE(char)
DECLARE_SET_FOR_TYPE(short)
DECLARE_SET_FOR_TYPE(int)
DECLARE_SET_FOR_TYPE(long)
DECLARE_SET_FOR_TYPE(int8_t)
DECLARE_SET_FOR_TYPE(int16_t)
DECLARE_SET_FOR_TYPE(int32_t)
DECLARE_SET_FOR_TYPE(int64_t)
DECLARE_SET_FOR_TYPE(uint8_t)
DECLARE_SET_FOR_TYPE(uint16_t)
DECLARE_SET_FOR_TYPE(uint32_t)
DECLARE_SET_FOR_TYPE(uint64_t)
DECLARE_SET_FOR_TYPE(double)
DECLARE_SET_FOR_TYPE(float)


#endif // __sti__sets_h__
