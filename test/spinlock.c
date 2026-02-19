
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <stdatomic.h>


#include "../spinlock.h"


typedef struct {
	
		
	mtx_t m;
	
	char work[4096];
	
} Data;

_Atomic volatile int n = 1;

thread_local int id;




_Atomic volatile int mm = 0;

int contested[129];



int start_fn_guarded(void* d_) {
	Data* d = (Data*)d_;
	
	id = atomic_fetch_add(&n, 1); // a reasonable thread id
	
	
	for(int i = 0; i < 1000; i++) {
		do {
			int zero = 0;
			int res = atomic_compare_exchange_strong_explicit(&mm, &zero, id, memory_order_acq_rel, memory_order_relaxed);
			if(!res) {
				contested[id]++;
//				printf("contested: I am %d, but %d has the lock\n", id, zero);
			}
			else break;
			
		} while(1);
		
		if(thrd_success == mtx_trylock(&d->m)) {
//			printf("%d foo %d\n", id, n);
			memset(d->work, i, sizeof(d->work));
			mtx_unlock(&d->m);	
		}
		else {
			printf("lock failed\n");
			exit(1);
		}
		
		atomic_store_explicit(&mm, 0, memory_order_release);
	}
	
	
	return 0;
}

int start_fn_mtx(void* d_) {
	Data* d = (Data*)d_;
	
	id = atomic_fetch_add(&n, 1); // a reasonable thread id
	
	
	for(int i = 0; i < 1000; i++) {
		
		if(thrd_success == mtx_lock(&d->m)) {
//			printf("%d foo %d\n", id, n);
			memset(d->work, i, sizeof(d->work));		
			mtx_unlock(&d->m);	
		}
		else {
			printf("lock failed\n");
			exit(1);
		}
		
	}
	
	
	return 0;
}

int start_fn_spinlock(void* d_) {
	Data* d = (Data*)d_;
	
	id = atomic_fetch_add(&n, 1); // a reasonable thread id
	
	
	for(int i = 0; i < 1000; i++) {
		do {
			int zero = 0;
			int res = atomic_compare_exchange_strong_explicit(&mm, &zero, id, memory_order_acq_rel, memory_order_relaxed);
			if(!res) {
//				contested[id]++;
//				printf("contested: I am %d, but %d has the lock\n", id, zero);
			}
			else break;
			
		} while(1);
		
	
		memset(d->work, i, sizeof(d->work));
//		printf("%d foo %d\n", id, n);		

		atomic_store_explicit(&mm, 0, memory_order_release);
	}
	
	
	return 0;
}



int main(int argc, char* argv[]) {

	int nthreads = 12;

	Data d;
	mtx_init(&d.m, mtx_plain);
	
	
	thrd_t threads[128];
	memset(contested, 0, sizeof(contested));
	
	
	for(int i = 0; i < nthreads; i++) {
		thrd_create(&threads[i], start_fn_mtx, &d);
	}
	

	for(int i = 0; i < nthreads; i++) {
		thrd_join(threads[i], NULL);
	}
	
	
	int total = 0;
	for(int i = 1; i < 129; i++) {
		total += contested[i];
	}

	printf("contested loops: %d\n", total);




	return 0;
}


