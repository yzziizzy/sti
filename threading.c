

#include <assert.h>
#include <errno.h>

#include "threading.h"
#include "misc.h"


ThreadPool* g_threadPool;


u64 QueueJob(Jerb* j) {
	ThreadPool_AddJob(g_threadPool, j);
	return 0;
}



int take_jerb_(void* fn, void* arg1, void* arg2, void* arg3, void* arg4, _Atomic int32_t* decr) {
	Jerb j = {
		.args = {arg1, arg2, arg3, arg4},
		.fn = fn,
		.priority = 1, // unused atm
		.decrement = decr,
	};
	
	return ThreadPool_AddJob(g_threadPool, &j);
}



static int thread_idler_fn(ThreadPool* p) {
//	printf("thread #%d\n", thread_id);
	
	atomic_fetch_add(&p->threadsActive, 1);
	
	while(1) {
		
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_nsec += 100000; // 100 microseconds
		
		errno = 0;
		sem_timedwait(&p->poolSema, &ts);
				
		if(p->shouldShutdown) {
			atomic_fetch_sub(&p->threadsActive, 1);
			return 0;
		}
	
		// do work or something
		Jerb j;
		int res = ThreadPool_ReadJob(p, &j);
		if(!res) {
			j.fn(j.args[0], j.args[1], j.args[2], j.args[3]);
			if(j.decrement) atomic_fetch_sub(j.decrement, 1);
		}
	}
	
	return 0;
}





int ThreadPool_Init(ThreadPool* p, uint32_t threadCnt) {
	
	// they start and then immediately idle
	if(sem_init(&p->poolSema, 0, 0)) {
		fprintf(stderr, "STI Thread Pool: Could not create semaphore.\n");
		return 1;
	}
	
	p->threadCnt = threadCnt;
	pcalloc(p->threads, threadCnt);
	FOR(i, threadCnt) {
		if(thrd_success != thrd_create(&p->threads[i], (void*)thread_idler_fn, p)) {
			fprintf(stderr, "STI Thread Pool: Failed to create thread\n");
			return 1;
		}
	}

	// TODO: track actual max queue fill level and adjust the buffer accordingly

	p->maxJobs = STI_THREADING_MAX_JOBS;
	p->jobQueue = calloc(1, sizeof(*p->jobQueue) * p->maxJobs);
	p->jobReadIndex = 0;
	p->jobWriteIndex = 0;
	p->jobSpinlock = 0;
	
	atomic_thread_fence(memory_order_release);
	
	return 0;
}


void ThreadPool_Destroy(ThreadPool* p) {
	free(p->jobQueue);
	sem_destroy(&p->poolSema);
}


void ThreadPool_ReleaseThreads(ThreadPool* p, uint32_t count) {
	
	// TODO: check something easy to see if there are threads waiting already or not
	//   syscalls might be bottlenecking small jobs

	FOR(i, count) sem_post(&p->poolSema);
}


int ThreadPool_AddJob(ThreadPool* p, Jerb* j) {
	
	assert(p->maxJobs > 0);
	assert(p->jobQueue);
	// w == r : empty queue
	// w == r-1 : full queue

	spin_lock(&p->jobSpinlock);
//	totalJobs++;
//	dbgv(totalJobs);
	
	int32_t r = p->jobReadIndex; 
	int32_t w = p->jobWriteIndex; 
	
	int32_t rmo = (r - 1 + p->maxJobs) % p->maxJobs;
	
	if(w == rmo) {
		spin_unlock(&p->jobSpinlock);
		dbg("job queue full")
		exit(1);
		return 1; // queue full
	}
	
	p->jobQueue[w] = *j;
	
	p->jobWriteIndex = (w + 1) % p->maxJobs;
	
	spin_unlock(&p->jobSpinlock);

	// TEMP
	ThreadPool_ReleaseThreads(p, 1);

	return 0;
}


int ThreadPool_ReadJob(ThreadPool* p, Jerb* j) {

	// w == r : empty queue
	// w == r-1 : full queue
	
	spin_lock(&p->jobSpinlock);
	
	int32_t r = p->jobReadIndex; 
	int32_t w = p->jobWriteIndex; 
	
	if(w == r) {
		spin_unlock(&p->jobSpinlock);
		return 1; // queue empty
	}
//	dbgv(totalJobs);
//	totalJobs--;
	
	*j = p->jobQueue[r]; // tHeY'rE tAkIn' 'Ar JeRbS!!!
	
	p->jobReadIndex = (r + 1) % p->maxJobs;
	
	spin_unlock(&p->jobSpinlock);

	return 0;
}


int ThreadPool_PendingJobCount(ThreadPool* p) {
	spin_lock(&p->jobSpinlock);	
	int cnt = (p->jobWriteIndex + p->maxJobs) - (p->jobReadIndex + p->maxJobs);
	spin_unlock(&p->jobSpinlock);
	
	return cnt;
}



