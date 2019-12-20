
#include <stddef.h> // ptrdiff_t, size_t
#include <stdlib.h> // [c|m|re]alloc, free
#include <string.h> // memcpy
#include <stdio.h> // printf

#include "sets.h"


// Pointer Sets


void PointerSet_print(PointerSet* ps) {
	
	printf("PointerSet %p (%ld items)\n", ps, ps->length);
	for(int i = 0; i < ps->length; i++) {
		printf(" %d: %p\n", i, ps->set[i]);
	}
}


size_t PointerSet_find_index(PointerSet* ps, void* p) {
	ptrdiff_t  R = ps->length - 1;
	ptrdiff_t L = 0;
	ptrdiff_t i;
	
	while(R - L > 0) {
		
		// midpoint
		i = L + ((R - L) / 2);
		if(ps->set[i] < p) {
			L = i + 1;
		}
		else if(ps->set[i] > p) {
			R = i - 1;
		}
		else {
			return i;
		}
	}
	
	return (ps->set[L] < p) ? L + 1 : L;
} 

void PointerSet_insert(PointerSet* ps, void* p) {
	
	if(ps->length == 0) {
		ps->alloc = 8;
		ps->set = calloc(1, ps->alloc * sizeof(*ps->set));
		
		ps->set[0] = p;
		ps->length++;
		return;
	}
	else if(ps->length + 1 <= ps->alloc) {
		ps->alloc *= 2;
		ps->set = realloc(ps->set, ps->alloc * sizeof(*ps->set));
	} 
	
	// find the slot
	size_t i = PointerSet_find_index(ps, p);
	if(ps->set[i] == p) return;

	memmove(ps->set + i + 1, ps->set + i, (ps->length - i) * sizeof(*ps->set));
	ps->set[i] = p;
	ps->length++;
}

int PointerSet_remove(PointerSet* ps, void* p) {
	if(ps->length == 0) return 0;
	
	size_t i = PointerSet_find_index(ps, p);
	if(ps->set[i] != p) return 0;
	
	memmove(ps->set + i, ps->set + i + 1, (ps->length - i - 1) * sizeof(*ps->set));
	ps->length--;
	return 1;
}

int PointerSet_exists(PointerSet* ps, void* p) {
	if(ps->length == 0) return 0;
	size_t i = PointerSet_find_index(ps, p);
	return (ps->set[i] == p);
}

PointerSet* PointerSet_alloc() {
	PointerSet* ps = malloc(sizeof(ps));
	ps->alloc = 0;
	ps->length = 0;
	
	return ps;
}

void PointerSet_init(PointerSet* ps) {
	ps->alloc = 0;
	ps->length = 0;
}

void PointerSet_free(PointerSet* ps) {
	free(ps->set);
	free(ps);
}

void PointerSet_destroy(PointerSet* ps) {
	free(ps->set);
	ps->alloc = 0;
	ps->length = 0;
}


int PointerSet_equal(PointerSet* a, PointerSet* b) {
	if(a->length != b->length) return 0;
	
	for(size_t i = 0; i < a->length; i++) {
		if(a->set[i] != b->set[i]) return 0;
	}
	
	return 1;
}


PointerSet* PointerSet_intersect(PointerSet* a, PointerSet* b) {
	PointerSet* c = malloc(sizeof(*c));
	
	c->alloc = a->length > b->length ? a->length : b->length;
	c->set = malloc(c->alloc * sizeof(*c->set));
	c->length = 0;
	
	size_t ci = 0;
	size_t bi = 0;
	size_t ai = 0;
	while(ai < a->length && bi < b->length) {
		void* ap = a->set[ai];
		void* bp = b->set[bi];
		if(ap == bp) {
			c->set[ci] = a->set[ai];
			c->length++;
			ai++; bi++; ci++; 
		}
		else if(ap > bp) {
			bi++;
		}
		else {
			ai++;
		}
	}
	
	return c;
}

PointerSet* PointerSet_union(PointerSet* a, PointerSet* b) {
	PointerSet* c = malloc(sizeof(*c));
	
	c->alloc = a->length + b->length;
	c->set = malloc(c->alloc * sizeof(*c->set));
	c->length = 0;
	
	size_t ci = 0;
	size_t bi = 0;
	size_t ai = 0;
	while(ai < a->length || bi < b->length) {
		void* ap = a->set[ai];
		void* bp = b->set[bi];
		if(ap == bp) {
			c->set[ci] = a->set[ai];
			c->length++;
			ai++; bi++; ci++; 
		}
		else if(ap > bp) {
			c->set[ci++] = b->set[bi];
			c->length++;
			bi++;
		}
		else {
			c->set[ci++] = a->set[ai];
			c->length++;
			ai++;
		}
	}
	
	return c;
}

void PointerSet_union_inplace(PointerSet* a, PointerSet* b) {
	
	a->alloc = a->length + b->length;
	a->set = realloc(a->set, a->alloc * sizeof(*a->set));
	
	// work backwards to fill in the data, then do one memmove
	size_t final_length = 0;
	ptrdiff_t bi = b->length - 1;
	ptrdiff_t ai = a->length - 1;
	ptrdiff_t wi = a->alloc - 1; // write index
	while(ai >= 0 && bi >= 0) {
		void* ap = a->set[ai];
		void* bp = b->set[bi];
		if(ap == bp) {
			a->set[wi] = a->set[ai];
			final_length++;
			ai--; bi--; wi--; 
		}
		else if(ap < bp) {
			a->set[wi--] = b->set[bi--];
			final_length++;
		}
		else {
			a->set[wi--] = a->set[ai--];
			final_length++;
		}
	};
	
	// finish off any remainder
	while(ai >= 0) {
		a->set[wi--] = a->set[ai--];
		final_length++;
	}
	while(bi >= 0) {
		a->set[wi--] = b->set[bi--];
		final_length++;
	}
	
	// the unioned data is at the end of the allocation. move it to the front.
	memmove(a->set, a->set + a->alloc - final_length, final_length * sizeof(*a->set));
	a->length = final_length;
}

PointerSet* PointerSet_difference(PointerSet* a, PointerSet* b) {
	PointerSet* c = malloc(sizeof(*c));
	
	c->alloc = a->length + b->length;
	c->set = malloc(c->alloc * sizeof(*c->set));
	c->length = 0;
	
	size_t ci = 0;
	size_t bi = 0;
	size_t ai = 0;
	while(ai < a->length || bi < b->length) {
		void* ap = a->set[ai];
		void* bp = b->set[bi];
		if(ap == bp) {
			ai++; bi++; 
		}
		else if(ap > bp) {
			c->set[ci++] = b->set[bi];
			c->length++;
			bi++;
		}
		else {
			c->set[ci++] = a->set[ai];
			c->length++;
			ai++;
		}
	}
	
	return c;
}


// Struct Sets

#define SS_EQ(ss, i, p) \
	ss->cmp(ss->set + i * ss->elem_size, p)

#define SS_CMP(a, b, ai, bi) \
	a->cmp(a->set + ai * a->elem_size, b->set + bi * b->elem_size)

#define SS_SET(ss, i, p) \
	memcpy(ss->set + i * ss->elem_size, p, ss->elem_size)

size_t StructSet_find_index(StructSet* ss, void* p) {
	ptrdiff_t  R = ss->length - 1;
	ptrdiff_t L = 0;
	ptrdiff_t i;
	
	while(R - L > 0) {
		
		// midpoint
		i = L + ((R - L) / 2);
		int n = SS_EQ(ss, i, &p);
		if(n < 1) {
			L = i + 1;
		}
		else if(n > 1) {
			R = i - 1;
		}
		else {
			return i;
		}
	}
	
	return (SS_EQ(ss, L, &p) < 0) ? L + 1 : L;
} 

int StructSet_insert(StructSet* ss, void* p) {
	
	if(ss->length == 0) {
		ss->alloc = 8;
		ss->set = calloc(1, ss->alloc * ss->elem_size);
		
		SS_SET(ss, 0, &p);
		ss->length++;
		return 0;
	}
	else if(ss->length + 1 <= ss->alloc) {
		ss->alloc *= 2;
		ss->set = realloc(ss->set, ss->alloc * ss->elem_size);
	} 
	
	// find the slot
	size_t i = StructSet_find_index(ss, p);
	if(SS_EQ(ss, i, &p) == 0) return 1;

	memmove(
		ss->set + (i + 1) * ss->elem_size, 
		ss->set + i * ss->elem_size, 
		(ss->length - i) * ss->elem_size
	);
	SS_SET(ss, i, &p);
	ss->length++;
	
	return 0;
}

int StructSet_remove(StructSet* ss, void* p) {
	if(ss->length == 0) return 0;
	
	size_t i = StructSet_find_index(ss, p);
	if(SS_EQ(ss, i, &p) != 0) return 0;
	
	memmove(
		ss->set + i * ss->elem_size, 
		 ss->set + (i + 1) * ss->elem_size,
		(ss->length - i - 1) * ss->elem_size
	);
	ss->length--;
	return 1;
}

int StructSet_exists(StructSet* ss, void* p) {
	if(ss->length == 0) return 0;
	size_t i = StructSet_find_index(ss, p);
	return SS_EQ(ss, i, &p) == 0;
}

StructSet* StructSet_alloc_(size_t elem_size, SetCmpFn cmp) {
	StructSet* ss = malloc(sizeof(ss));
	StructSet_init(ss, elem_size, cmp);
	return ss;
}

void StructSet_free(StructSet* ss) {
	free(ss->set);
	free(ss);
}

void StructSet_destroy(StructSet* ss) {
	free(ss->set);
	ss->alloc = 0;
}

StructSet* StructSet_intersect(StructSet* a, StructSet* b) {
	StructSet* c = malloc(sizeof(*c));
	
	c->alloc = a->length > b->length ? a->length : b->length;
	c->set = malloc(c->alloc * c->elem_size);
	c->length = 0;
	c->elem_size = a->elem_size;
	c->cmp = a->cmp;
	
	size_t ci = 0;
	size_t bi = 0;
	size_t ai = 0;
	while(ai < a->length && bi < b->length) {
		int n = SS_CMP(a, b, ai, bi);
		if(n == 0) {
			SS_SET(c, ci, a->set + ai * a->elem_size);
			c->length++;
			ai++; bi++; ci++; 
		}
		else if(n > 0) {
			bi++;
		}
		else {
			ai++;
		}
	}
	
	return c;
}

StructSet* StructSet_union(StructSet* a, StructSet* b) {
	StructSet* c = malloc(sizeof(*c));
	
	c->alloc = a->length + b->length;
	c->set = malloc(c->alloc * c->elem_size);
	c->length = 0;
	c->elem_size = a->elem_size;
	c->cmp = a->cmp;
	
	size_t ci = 0;
	size_t bi = 0;
	size_t ai = 0;
	while(ai < a->length || bi < b->length) {
		int n = SS_CMP(a, b, ai, bi);
		if(n == 0) {
			SS_SET(c, ci, a->set + ai * a->elem_size);
			c->length++;
			ai++; bi++; ci++; 
		}
		else if(n > 0) {
			SS_SET(c, ci, b->set + bi * b->elem_size);
			c->length++;
			bi++;
		}
		else {
			SS_SET(c, ci, a->set + ai * a->elem_size);
			c->length++;
			ai++;
		}
	}
	
	return c;
}

StructSet* StructSet_difference(StructSet* a, StructSet* b) {
	StructSet* c = malloc(sizeof(*c));
	
	c->alloc = a->length + b->length;
	c->set = malloc(c->alloc * c->elem_size);
	c->length = 0;
	c->elem_size = a->elem_size;
	c->cmp = a->cmp;
	
	size_t ci = 0;
	size_t bi = 0;
	size_t ai = 0;
	while(ai < a->length || bi < b->length) {
		int n = SS_CMP(a, b, ai, bi);
		if(n == 0) {
			ai++; bi++; 
		}
		else if(n > 0) {
			SS_SET(c, ci, b->set + bi * b->elem_size);
			c->length++;
			bi++;
		}
		else {
			SS_SET(c, ci, a->set + ai * a->elem_size);
			c->length++;
			ai++;
		}
	}
	
	return c;
}



// Sets for base types

DEFINE_SET_FOR_TYPE(char, "%c")
DEFINE_SET_FOR_TYPE(short, "%d")
DEFINE_SET_FOR_TYPE(int, "%d")
DEFINE_SET_FOR_TYPE(long, "%ld")
DEFINE_SET_FOR_TYPE(uint8_t, "%d")
DEFINE_SET_FOR_TYPE(uint16_t, "%d")
DEFINE_SET_FOR_TYPE(uint32_t, "%d")
DEFINE_SET_FOR_TYPE(uint64_t, "%ld")
DEFINE_SET_FOR_TYPE(float, "%f")
DEFINE_SET_FOR_TYPE(double, "%f")


