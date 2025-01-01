#ifndef __sti__spinlock_h__
#define __sti__spinlock_h__

#include <stdatomic.h>

static inline void spin_lock(_Atomic u32* sl) {
	do {
		int zero = 0;
		int res = atomic_compare_exchange_strong_explicit(sl, &zero, 1, memory_order_acq_rel, memory_order_relaxed);
		if(res) return;
		
		for(volatile int i = 0; i < 50; i++); // waste some time; TODO: make sure -O3 doesn't optimize out this whole statement
	} while(1);
}

static inline void spin_unlock(_Atomic u32* sl) {
	atomic_store_explicit(sl, 0, memory_order_release);
}


#endif //__sti__spinlock_h__
