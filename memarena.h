#ifndef __sti__memarena_h__
#define __sti__memarena_h__

// Public Domain.


#include <stdlib.h>
#include <stdint.h>

// MemArena is a FAST, non-thread-safe stack/arena allocator for arbitrary size objects.
//  There are no system calls during allocation or freeing, though the kernel may run
//  interrupts when a new page of virtual memory is written to for the first time.

// This is written for Linux, version >= not ancient.

// This code assumes that size_t is at least big enough to store a pointer. 
//   Weird systems get wrecked.

// Does not:
//   Handle threads. Manage synchronization yourself.
//   Grow the arena dynamically. Request enough virtual address space from the start.
//   Reserve physical memory with the OS. It's purely virtual until you use it.

// Global Compile Flags:
// 
// STI_MEMARENA_NO_HWM
//    Do not store or track the high water mark. _getHWM() will always return 0.
//    
// STI_MEMARENA_NO_MALLOC_BOUNDS_CHECK
//    Do not check for arena overflow on malloc. Use at your peril.




typedef struct MemArena{
	
	size_t maxSize;
	void* top; // ptr to the first free byte
	void* arena; // base pointer
	
#ifndef STI_MEMARENA_NO_HWM
	void* hwm; // high water mark
#endif
} MemArena;



// THIS ***IS*** WHAT YOU ARE LOOKING FOR

// for allocation of chunks of memory from the pool itself
void* MemArena_malloc(MemArena* ma, size_t size);
void* MemArena_calloc(MemArena* ma, size_t size);

// does what it says, no tricks
void* MemArena_alignedAlloc(MemArena* ma, size_t alignment, size_t size);

// zero on success, non-zero otherwise. RTFS for the "codes".
int MemArena_freeAfter(MemArena* ma, void* ptr);

// resets the entire arena, freeing all memory inside it
void MemArena_reset(MemArena* ma);

size_t MemArena_getUsedSize(MemArena* ma);
size_t MemArena_getFreeSize(MemArena* ma);
size_t MemArena_getHighWaterMark(MemArena* ma); // used, in bytes







// ...





// ***NOT*** WHAT YOU ARE LOOKING FOR
// for allocation and initialization of the MemArena itself

// malloc() a MemArena struct then call _initArena on it
MemArena* MemArena_allocArena(size_t maxsize);

// initialize the struct, including system resources
void MemArena_initArena(MemArena* ma, size_t maxSize);

// frees internal resources but does NOT call free() on ma
void MemArena_destroyArena(MemArena* ma);

// frees internal resources then calls free() on ma
void MemArena_freeArena(MemArena* ma);





#endif // __sti__memarena_h__
