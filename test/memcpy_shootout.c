#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>


void* memcpy_1(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_2(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_2b(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_3(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_4(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_5(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_6(void* restrict dest, const void* restrict src, size_t n);
void* memcpy_6_slower(void* restrict dest, const void* restrict src, size_t n);


#define FN_LIST \
	X(memcpy, glibc) \
	X(memcpy_1, naive) \
	X(memcpy_6, xaligned_avx) \


//	X(memcpy_2b, unaligned_avx)
//	X(memcpy_2, unaligned_avx_a) 
//	X(memcpy_5, aligned_avx) 
//	X(memcpy_4, avx_uncached) 
	
	
	
	
enum {
#define X(a,...) ORD_##a,
	FN_LIST
#undef X
	DEPTH
};

char* fn_names[] = {
#define X(a,b,...) #b,
	FN_LIST
#undef X
};


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
	
	int nsizes = 8;
	
	// create some test memory chunks
	int* sizes = malloc(sizeof(*sizes) * nsizes);
	char** datas = malloc(sizeof(*datas) * nsizes);
	
	long maxSize = 1024 * 1024 * 16;
	
	for(int i = 0; i < nsizes; i++) {
//		sizes[i] = 8 + ((double)rand() / (double)RAND_MAX) * 1024*1024;
		sizes[i] = ((double)(i + 1) / (double)nsizes) * maxSize;
		datas[i] = aligned_alloc(32, sizes[i]);
	}
	
	
	// fill in some dummy data
	for(int i = 0; i < nsizes; i++) {
		for(int j = 0; j < sizes[i]; j++) {
			datas[i][j] = rand();
		}
		fprintf(stderr, "\rinitializing: %d / %d", i + 1, nsizes);
		fflush(stderr);
	}
	
	fprintf(stderr, "\n ");
	
	// somewhere big enough to hold any of the chunks	
	dest = aligned_alloc(32, maxSize+8+ 32); 
	
	
	
//	
//	for(int i = 0; i < nsizes; i++) {
//		for(int j = 0; j < 32; j++) {
//			memcpy_5(dest + j, datas[i] + j, sizes[i]);
//		}
//	}	
//	
//	
	
	
	int reps = argc < 100 ? 1000 : argc;
	
	double* finals = calloc(1, sizeof(*finals) * DEPTH * nsizes);
	
	for(int i = 0; i < nsizes; i++) {
		for(int al = 0; al < 1; al++) {
		
			fprintf(stderr, "\rtesting: %d / %d", al + 1 + (i * 32), nsizes * 32);
			fflush(stderr);
			
			#define X(name,...) \
				reset_cache(); \
				double start_##name = getCurrentTime(); \
				\
				for(int k = 0; k < reps; k++) { \
					name(dest, datas[i], sizes[i]); \
				} \
				\
				double end_##name = getCurrentTime(); \
				double final_##name = end_##name - start_##name; \
				finals[ORD_##name + DEPTH * i] += final_##name; \
				
				//printf(#name ": %d bytes, %.15fs\n", sizes[i], final_##name);
				
				FN_LIST
			#undef X
			
		}
	}
	fprintf(stderr, "\n ");
	
	double totals[DEPTH];
	for(int i = 0; i < DEPTH; i++) {
		totals[i] = 0;
	}
	
	fprintf(stderr, "sizes: ");
	for(int i = 0; i < nsizes; i++) {
		
		fprintf(stderr, "%d, ", sizes[i]);
		for(int j = 0; j < DEPTH; j++) {
			totals[j] += finals[j + DEPTH * i];
		}
	}
	fprintf(stderr, "\n\n");
	
	int maxname = 0;
	for(int i = 0; i < DEPTH; i++) {
		if(strlen(fn_names[i]) > maxname) maxname = strlen(fn_names[i]);
	}	
	
	for(int i = 0; i < DEPTH; i++) {
		fprintf(stderr, "%s: %.*s%.15f\n", fn_names[i], (int)(maxname - strlen(fn_names[i])), "               ", totals[i]);
	}
	
	
	// sizes header, first col empty
	printf(",");
	for(int i = 0; i < nsizes; i++) {
		printf("%d%s", sizes[i], i == nsizes - 1 ? "" : ",");
	}
	printf("\n");

	// time data
	for(int j = 0; j < DEPTH; j++) {
		
		printf("\"%s\",", fn_names[j]);
		for(int i = 0; i < nsizes; i++) {
			printf("%.15f%s", finals[j + DEPTH * i], i == nsizes - 1 ? "" : ",");
		}
		printf("\n");
	}
	
	
//	#define X(name, desc,...) printf("%s: %.15f\n", #desc, totals[ORD_##name]);
//		FN_LIST
//	#undef X


	return 0;
}






