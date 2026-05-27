#ifndef __sit__threading_h__
#define __sit__threading_h__

#include <stdint.h>
#include <threads.h>
#include <semaphore.h>

#include "spinlock.h"
#include "slot.h"
#include "vec.h"


#ifndef STI_THREADING_MAX_JOBS
	#define STI_THREADING_MAX_JOBS  (256*1024)
#endif


// an individual task
typedef struct Jerb {
	uint32_t priority;
	_Atomic uint32_t status;
	_Atomic int32_t* decrement; // automatically decremented on completion
	
	void* args[4];
	void (*fn)(void*, void*, void*, void*);
} Jerb;



typedef struct ThreadPool {

	sem_t poolSema;
	uint32_t threadCnt;
	thrd_t* threads;
	
	Jerb* jobQueue;
	_Atomic uint32_t jobSpinlock; 
	_Atomic uint32_t shouldShutdown; 
	_Atomic int32_t threadsActive; 
	uint32_t jobReadIndex;
	uint32_t jobWriteIndex;
	uint32_t maxJobs;
	
	
} ThreadPool;



// just run something and don't worry about it too much
// returns 0 on successfully queued, 1 if the queue is full

#define take_jerb(...) take_jerb_N(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define take_jerb_N(n, ...) CAT(take_jerb_, n)(__VA_ARGS__)
#define take_jerb_1(fn)                 take_jerb_(fn, NULL, NULL, NULL, NULL, NULL)
#define take_jerb_2(fn, a1)             take_jerb_(fn, a1, NULL, NULL, NULL, NULL)
#define take_jerb_3(fn, a1, a2)         take_jerb_(fn, a1, a2, NULL, NULL, NULL)
#define take_jerb_4(fn, a1, a2, a3)     take_jerb_(fn, a1, a2, a3, NULL, NULL)
#define take_jerb_5(fn, a1, a2, a3, a4) take_jerb_(fn, a1, a2, a3, a4, NULL)

// should be done with _Generic, but meh
#define take_jerb_decr(...) take_jerb_decr_N(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define take_jerb_decr_N(n, ...) CAT(take_jerb_decr_, n)(__VA_ARGS__)
#define take_jerb_decr_2(d, fn)                 take_jerb_(fn, NULL, NULL, NULL, NULL, d)
#define take_jerb_decr_3(d, fn, a1)             take_jerb_(fn, a1, NULL, NULL, NULL, d)
#define take_jerb_decr_4(d, fn, a1, a2)         take_jerb_(fn, a1, a2, NULL, NULL, d)
#define take_jerb_decr_5(d, fn, a1, a2, a3)     take_jerb_(fn, a1, a2, a3, NULL, d)
#define take_jerb_decr_6(d, fn, a1, a2, a3, a4) take_jerb_(fn, a1, a2, a3, a4, d)

int take_jerb_(void* fn, void* arg1, void* arg2, void* arg3, void* arg4, _Atomic int32_t* decr);




// fiddle with all the params
uint64_t QueueJob(Jerb* j);


// returns 0 on success, 1 otherwise (thread pool will not function)
int ThreadPool_Init(ThreadPool* p, uint32_t threadCnt);
void ThreadPool_Destroy(ThreadPool* p);

void ThreadPool_ReleaseThreads(ThreadPool* p, uint32_t count);
int ThreadPool_AddJob(ThreadPool* p, Jerb* j);
int ThreadPool_ReadJob(ThreadPool* p, Jerb* j);
int ThreadPool_PendingJobCount(ThreadPool* p);


extern ThreadPool* g_threadPool;


static void nano_sleep(u64 nanoseconds) {
	struct timespec ts;
	ts.tv_sec = nanoseconds / 1000000000;
	ts.tv_nsec = nanoseconds % 1000000000;
	nanosleep(&ts, &ts);
}


#endif // __sit__threading_h__
