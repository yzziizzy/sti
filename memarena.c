// Public Domain.

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/mman.h>

#include "memarena.h"






MemArena* MemArena_allocArena(size_t maxsize) {
	MemArena* ma;
	
	ma = calloc(1, sizeof(*ma));
	
	MemArena_initArena(ma, maxsize);
	
	return ma;
}



void MemArena_initArena(MemArena* ma, size_t maxSize) {
	ma->maxSize = maxSize;
	
	int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
	ma->arena = mmap(NULL, maxSize, PROT_READ | PROT_WRITE, flags, 0, 0);
	if(ma->arena == MAP_FAILED) {
		fprintf(stderr, "mmap failed in %s: %s\n", __func__, strerror(errno));
		exit(1);
	}
	
	ma->top = ma->arena;
}


void MemArena_destroyArena(MemArena* ma) {
	
	munmap(ma->arena, ma->maxSize);
	
	ma->maxSize = 0;
	ma->arena = NULL;
	ma->top = NULL;
}


void MemArena_freeArena(MemArena* ma) {
	MemArena_destroyArena(ma);
	free(ma);
}



// ----------------------------------------




void* MemArena_malloc(MemArena* ma, size_t size) {
#ifndef STI_MEMARENA_NO_MALLOC_BOUNDS_CHECK
	if((char*)ma->top >= (char*)ma->arena + ma->maxSize + size) {
		fprintf(stderr, "MemArena overflowed size of %ld\n", ma->maxSize);
		return NULL;
	}
#endif
	
	void* t = ma->top;
	ma->top = (char*)ma->top + size;  
	
#ifndef STI_MEMARENA_NO_HWM
	ma->hwm = (ma->hwm < ma->top) ? ma->top : ma->hwm;
#endif
	
	return t;
}


void* MemArena_calloc(MemArena* ma, size_t size) {
	void* p = MemArena_malloc(ma, size);
	memset(p, 0, size);
	return p;
}


void* MemArena_alignedAlloc(MemArena* ma, size_t alignment, size_t size) {
	size_t t = (size_t)ma->top;
	size_t rem = t % alignment;
	size_t o = alignment - rem;
	void* p = (char*)ma->top + o;
	ma->top = (char*)p + size;
	
	return p;
}


// void* MemArena_realloc(MemArena* ma, size_t size) {
// 	
// }


// zero on success, non-zero otherwise
int MemArena_freeAfter(MemArena* ma, void* ptr) {
	// check to see if this pointer is inside the arena
	if(ptr < ma->arena) return 1;
	if(ptr >= ma->top) return 2;
	
	ma->top = ptr;
	
	return 0;
}

void MemArena_reset(MemArena* ma) {
	ma->top = ma->arena;
	ma->hwm = ma->arena;
}



size_t MemArena_getUsedSize(MemArena* ma) {
	return (size_t)ma->top - (size_t)ma->arena;
}

size_t MemArena_getFreeSize(MemArena* ma) {
	return ma->maxSize - ((size_t)ma->top - (size_t)ma->arena);
}

size_t MemArena_getHighWaterMark(MemArena* ma) {
#ifndef STI_MEMARENA_NO_HWM
	return (size_t)ma->hwm - (size_t)ma->arena;
#else
	return 0;
#endif
}
