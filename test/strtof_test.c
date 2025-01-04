#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <float.h>
#include <math.h>


float strtof_1(char* s, char** end);

long cacheSize;
long halfCacheSize;

char* dest; // to help keep gcc from optimizing it away
char* cachebuster;


double getCurrentTime() { // in seconds
	double now;
	struct timespec ts;
	static double offset = 0;
	
	// CLOCK_MONOTONIC_RAW is linux-specific.
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	
	now = (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
	if(offset == 0) offset = now;
	
	return now - offset;
}

// touch a large, contiguous section of memory that will overlap with the entire cache (hopefully, assuming there's no vma magic here) 
void reset_cache() {
	for(size_t i = 0; i < cacheSize; i++) {
		cachebuster[i]++;
	}
}

int main(int argc, char* argv[]) {

	srand(101);
	
	cacheSize = sysconf(_SC_LEVEL3_CACHE_SIZE);
	long l4 = sysconf(_SC_LEVEL4_CACHE_SIZE);
	if(l4 > cacheSize) cacheSize = l4;
	halfCacheSize = cacheSize / 2;
	
	cachebuster = calloc(1, cacheSize);
	
	fprintf(stderr, "cache size: %ld\n", cacheSize);


/*
	char buf[300];
	
	for(unsigned long i = 0; i < 0xffffffff; i++) {
		union { unsigned int n; float f; } u;
//		u.n = i;
		u.n = rand();
		float o = u.f;
//		if(o >= 1 || o <= 0 || o < FLT_MIN) continue;
//		
//	for(unsigned long i = 0; i < 0xffffffff; i++) {
//		unsigned int u = rand();
//		float o = *(float*)&u;
//		
		if(!isfinite(o)) continue;
//		if(o >= 1 || o <= 0 || o < FLT_MIN) continue;
		
//		o = 3e-32f;
//	for(unsigned long i = 1; i < 10000; i++) {
//		float o = (float)i / 7; 
		
		
		sprintf(buf, "%.9g", o); // %.9g for canonical representation
//		sprintf(buf, "%.70f", o); // %.9g for canonical representation
		
		float f = strtof_1(buf, NULL);
		if(f != o) {
			printf("%.9g [%s] != %.9g\n", o, buf, f);
			exit(1);
		}
		else {
//			printf("%.15f == %.15f\n", o, f);
		}
		
		if(i % 0xffffff == 0) printf("%ld\n", i);
	}

	exit(1);
	
	//*/


	int tries = 100000;

	char buf2[tries][70];

	for(unsigned long i = 0, n = 0; i < 0xffffffff && n < tries; i += 0xffff, n++) {
		union { unsigned int n; float f; } u;
//		u.n = i;
		u.n = rand();
		float o = u.f;
	
		if(!isfinite(o)) continue;
		n++;
		
		sprintf(buf2[n], "%.9g", o);
//		sprintf(buf2[n], "%.50f", o);
//		sprintf(buf2[n], "%.2f", o);
	}


	reset_cache();
	double start_mine = getCurrentTime();
	float sum = 0;
	for(int i = 0; i < tries; i++) {
		sum += strtof_1(buf2[i], NULL);
	}
	double end_mine = getCurrentTime();

	
	reset_cache();
	double start_libc = getCurrentTime();
	float sum2 = 0;
	for(int i = 0; i < tries; i++) {
		sum2 += strtof(buf2[i], NULL);
	}
	double end_libc = getCurrentTime();

	printf("libc: %.15f\n", end_libc - start_libc);
	printf("mine: %.15f\n", end_mine - start_mine);

	printf("%.9g, %.9g\r", sum, sum2);

	exit(0);
}
	


