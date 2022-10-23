// Public Domain.

#define _GNU_SOURCE     /* Expose declaration of tdestroy() */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <math.h>
#include <search.h> // for other testing
#include <time.h>

#include <float.h> // for float limts of conversion testing


#include "sti.h"
#include "./string.h"

// optional utilities
#include "ini.h"




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


char** generate_random_strings(size_t len, size_t block_size);

// temp, will be static later
int flt_r_cvt_str(float f, int base, char* buf, char* charset);
 int int_r_cvt(int64_t n, int base, int upper, char* buf) ;

 
int iprintf(char* fmt, ...);
 
int isnprintfv(char* out, ptrdiff_t out_sz, char* fmt, void** args);

// static void nothin(void* );
// static void nothin(void* b) { (void)b; }
// static int intcomp(const void* a, const void* b);
// static int intcomp(const void* a, const void* b) {
// 	int ai = *(uint64_t*)a;
// 	int bi = *(uint64_t*)b;
// 	return bi - ai;
// }

//void mergesort_strings(char** arr, int len, int bs, int (*compar)(const void *, const void *));

static int str_sort(const void* a, const void* b) {
	char* c = *((char**)a);
	char* d = *((char**)b);
	return strcmp(c, d);
}

struct trav_data {
	char* arr;
	char* cpy;
	int n;
	int bs;
};

static long rb_trav_fn(char* key, void* data, void* user_data) {
	struct trav_data* td = (struct trav_data*)user_data;
	memcpy(
		td->cpy + (td->n * td->bs),
		td->arr + ((long)data * td->bs),
		td->bs
	);
	td->n++;
	
	(void)key;
	return 0;
}

void bench_sort(int n, int bs) {
	double s, e;
	char** arr = generate_random_strings(n, bs);
	char* arr2 = malloc(bs * n);
	char** arr3 = malloc(bs * n);
	memcpy(arr2, arr, bs * n);
	memcpy(arr3, arr, bs * n);
	
// 	qsort(arr3, n, sizeof(*arr2), str_sort);
// 	qsort(arr2, n, sizeof(*arr2), str_sort);
// 	qsort(arr, n, sizeof(*arr2), str_sort);
	
	printf("\nN = %d\n", n);
	
	s = getCurrentTime();
//	mergesort_strings(arr, n, bs, str_sort);
	e = getCurrentTime();
	printf(" Merge Sort:  %fms\n", (e - s) * 1000);
	
	
	s = getCurrentTime();
	RB(int) rb;
	char* cpy = malloc(bs * n);
	struct trav_data td = {arr2, cpy, 0, bs};
	RB_init(&rb);
	for(intptr_t i = 0; i < n; i++) 
		RB_insert(&rb, 
			*((char**)(arr2 + i*bs))
			, i);
	RB_traverse(&rb, rb_trav_fn, &td);
	RB_trunc(&rb);
	memcpy(arr2, cpy, bs * n);
	free(cpy);
	e = getCurrentTime();
	printf(" RB Traverse: %fms\n", (e - s) * 1000);
	
	
	s = getCurrentTime();
	qsort(arr3, n, bs, str_sort);
	e = getCurrentTime();
	printf(" libc qsort:  %fms\n", (e - s) * 1000);
	
	free(arr);
	free(arr2);
	free(arr3);
}


int intcmp(void* a_, void* b_) {
	int* a = (int*)a_;
	int* b = (int*)b_;
	return *b - *a;
}

int ini_callback(char* section, char* key, char* value, void* user_data) {
	(void)user_data;
	if(value)
		printf("[%s] '%s' = '%s'\n", section, key, value);
	else
		printf("[%s] '%s' = NULL\n", section, key);
	return 0;
}
	


int main(int argc, char* argv[]) {
	char c;
	char test_sets = 0;
	char test_vec = 0;
	char test_fs = 0;
	char test_b_vs_t = 0;
	char test_rpn = 0;
	char test_iprintf = 0;
	char test_Iprintf = 0;
	char test_commas = 0;
	char test_ring = 0;
	char test_sort = 0;
	char test_talloc = 0;
	char test_heap = 0;
	char test_range = 0;
	char test_ini = 0;
	char test_hash = 0;
	
	
	//char* source = readWholeFile("./objtext.txt", NULL);
	//objdp_text(source);
	
// 	uint32_t* u = utf8_to_utf32("bafoooo", NULL); 
// 	uint32_t* v = utf8_to_utf32("fooaarZZZ", NULL); 
// 	uint32_t* w = malloc(500);
// 	
//  	printf("d: %d\n", strcspn32(u, utf8_to_utf32("ba", NULL)));
// // 	printf("r: %d\n", strspn("foooobar", "fo"));
	
	
// 	uint32_t* x = strchrnul32(w, 'a');
	
// 	printf("c: %d\n", );
	
// 	for(int i = 0; x[i]; i++) printf("%c", x[i]);

	while ((c = getopt (argc, argv, "AtchsSvf1piInrR")) != -1) {
		switch(c) {
			case 't': test_talloc = 1; break;
			case 's': test_sets = 1; break;
			case 'v': test_vec = 1; break;
			case 'S': test_sort = 1; break;
			case 'f': test_fs = 1; break;
			case '1': test_b_vs_t = 1; break;
			case 'p': test_rpn = 1; break;
			case 'i': test_iprintf = 1; break;
			case 'I': test_Iprintf = 1; break;
			case 'n': test_ini = 1; break;
			case 'c': test_commas = 1; break;
			case 'r': test_ring = 1; break;
			case 'R': test_range = 1; break;
			case 'h': test_heap = 1; break;
			case 'A': test_hash = 1; break;
		}
	}
	
	

	
	if(test_range) {

			
		
		
	
	}
	if(test_hash) {
		int fooval;
		int* foovalp;
		
		HT(int) foo;
		
		HT(int, int, Sizeof) bar;
		
		HT_init(&foo, 32);
		HT_init(&bar, 32);
		
		HT_set(&foo, "foo", &fooval);
		HT_setn(&bar, 64, &fooval);
	//	HT_getp(&foo, "foo", &foovalp);
		
	//	HT_set(&bar, 500, fooval);
	//	HT_getp(&bar, 500, &foovalp);
		
	}
	if(test_ini) {

		ini_read("./initest.ini", ini_callback, (void*)1337);	
	}
	
	if(test_heap) {
		HEAP(char*) h;
		HEAP_init(&h, str_sort);
		
		int i = 80;
		char* k;
		int len = 8000;
		
		char** arr = generate_random_strings(len, 8);
		
		for(i = 0; i < len; i++)
			HEAP_insert(&h, &arr[i]); 
// 		heap_insert_(&h, &j, intcmp, 4); 
// 		heap_insert_(&h, &k, intcmp, 4); 
// 		heap_insert_(&h, &l, intcmp, 4); 
// 		heap_insert_(&h, &m, intcmp, 4); 
// 		heap_print_(&h, 4);
		printf("\n");
		
		
		for(i = 0; i < len; i++) {
			HEAP_pop(&h, &k); 
// 			heap_print_(&h, 4);
			printf("%s\n", k);
		}
		
		HEAP_free(&h);
		/*
		heap_pop_(&h, &k, intcmp, 4); 
		heap_print_(&h, 4);
		printf("\n");
		
		heap_pop_(&h, &k, intcmp, 4); 
		heap_print_(&h, 4);*/
		
		return 0;
	}
	
	
	if(test_talloc) {
		/*
		void* a = talloc(NULL, 503);
		void* aa = talloc(a, 3);
		void* ab = talloc(a, 53);
		void* ac = talloc(a, 5);
		void* aba = talloc(ab, 45);
		void* abb = talloc(ab, 45);
		void* abc = talloc(ab, 45);
		
		trealloc(ab, 7435);
		
		tfree(a);*/
		return 0;
	}
	
	if(test_sort) {
		int block_size = 4*16;
		bench_sort(10, block_size);
		bench_sort(100, block_size);
		bench_sort(1000, block_size);
		bench_sort(10000, block_size);
		bench_sort(100000, block_size);
		bench_sort(1000000, block_size);
		
	}
	
	
	if(test_ring) {
		RING(int) r;
		
		RING_INIT(&r, 10);
		
		for(int j = 0; j < 15; j++) {
			RING_PUSH(&r, 50 + j);
		}
		
		
		
		RING_EACH(&r, i, v) {
			printf("each: %ld - %d\n", i, v);
		}
		printf("--\n");
		
		//for(int i = 0; i < 10; i++) {
		//	printf("%d: %d\n", i, r.data[i]);
		//}
		
//		int a = -99;
		//RING_POP(&r, a);
		//printf("\na: %d, len: %ld\n", a, r.len);
	
		RING_RM(&r, 9);	
		
//		int b = -99;
		//RING_POP(&r, b);
		//printf("\nb: %d, len: %ld\n", b, r.len);
		
		//RING_PUSH(&r, 70);
		
//		int c = -99;
		//RING_POP(&r, c);
		//printf("\nc: %d, len: %ld\n", c, r.len);
		
		RING_EACH(&r, i, v) {
			printf("each: %ld - %d\n", i, v);
		}
		
	}
	
	
	if(test_Iprintf) {
		char buffer[256] = "zzzzzzzzzzzz";
		size_t n;
		int i = 77;
		int* pi = &i;
		int** ppi = &pi;
		double dd = 3.5;
		uint64_t nn = *((uint64_t*)&dd);
		uint64_t n1, n2;
		(void)pi;
		(void)ppi;
		
		uint64_t iargs[] = {
			3,
			(uint64_t)ppi,
			
			(uint64_t)&n1,
			nn,
			(uint64_t)&dd,
			-500000000,
			(uint64_t)"string test",
			(uint64_t)&n2,
		};
		
// 		printf("%d\n", snprintf(buffer, 100, "%ld", 123456l));
		n = isnprintfv(buffer, 256, "a %>>.*ld%n b %.3f c %p d %d e %s f-%n-g", (void**)iargs);
		
		printf("\n\n%s\n%ld\n%p\n%ld, %ld\n", buffer, n, (void*)&dd, n1, n2);
	}
	
	
	if(test_iprintf) {
		
		for(int i = 0; i < 24; i++) {
			long ii = 5;
			uint64_t ijh = 0;
			uint64_t ijl = 1;
			for(int n = 0; n < i; n++) ii *= 5;
			for(int n = 23; n > i; n--) {
				uint64_t two, four;
// 				uint64_t ci = 0, co = 0;
				ijh += __builtin_add_overflow(ijl, ijl, &two);
				ijh += __builtin_add_overflow(two, two, &four);
				ijh += __builtin_add_overflow(four, four, &ijl);
				ijh += __builtin_add_overflow(two, ijl, &ijl);
				ijh *= 10;
			}
			printf("% .2d - %.25f - %lu %lu - % .*ld\n", i, 1.0/(2<<i), ijh, ijl, i+1, ii);
		}
		
		
		char buf[100];
		uint32_t N = 0x00000001;
		float F = *((float*)&N);
		float f = F;// -0x1.fffffep+127;
		printf("%a\n", f);
		uint32_t d = *((uint32_t*)&f);
		int n = int_r_cvt(d, 2, 0, buf);
		printf("%.*s\n", n, buf);
		flt_r_cvt_str(f, 10, buf, "0123456789abcdef");
// 		printf("%.*s\n", n, buf);
		
	}
	
	if(test_rpn) {
		sti_op_prec_rule rules[] = {
			{"",   0, STI_OP_ASSOC_NONE,  0},
			{"+",  1, STI_OP_ASSOC_LEFT,  2},
			{"-",  1, STI_OP_ASSOC_LEFT,  2},
			{"*",  2, STI_OP_ASSOC_LEFT,  2},
			{"**", 3, STI_OP_ASSOC_LEFT,  2},
			{"/",  2, STI_OP_ASSOC_LEFT,  2},
			{"(",  8, STI_OP_OPEN_PAREN,  0},
			{")",  8, STI_OP_CLOSE_PAREN, 0},
			{"[",  9, STI_OP_OPEN_PAREN,  0},
			{"]",  9, STI_OP_CLOSE_PAREN, 0},
			{NULL, 0, 0, 0},
		};

		char* infix[] = {
			"2",
			"*",
			"4",
// 			"1",
// 			"*",
// 			"(",
// 			"2",
// 			"+",
// 			"3",
// 			")",
// 			"/",
// 			"[",
// 			"4",
// 			"-",
// 			"5",
// 			"]",
// 			"*",
// 			"2",
			NULL,
		};
		
		char** rpn;
		size_t len;
		
		infix_to_rpn(rules, infix, &rpn, &len);
		
		printf("answer: %ld \n", rpn_eval_int_str(rpn));
		
		while(*rpn) {
			printf(" %s \n", *rpn);
			rpn++;
		}
		
	}
		
		
	if(test_sets) {
		PointerSet* ps = calloc(1, sizeof(*ps));
		PointerSet* ps2 = calloc(1, sizeof(*ps2));
		
		PointerSet_insert(ps, (void*)0x00004);
		PointerSet_insert(ps, (void*)0x00002);
		PointerSet_insert(ps, (void*)0x00003);
		PointerSet_insert(ps, (void*)0x00008);
		PointerSet_insert(ps, (void*)0x00008);
		PointerSet_insert(ps, (void*)0x00001);
		PointerSet_insert(ps2, (void*)0x00003);
		PointerSet_insert(ps2, (void*)0x00008);
		PointerSet_insert(ps2, (void*)0x00007);
		PointerSet_insert(ps2, (void*)0x00002);
		PointerSet_insert(ps2, (void*)0x00008);
		PointerSet_print(ps);
		PointerSet_print(ps2);
		PointerSet* ps3 = PointerSet_intersect(ps, ps2);
		PointerSet_print(ps3);
		PointerSet* ps4 = PointerSet_union(ps, ps2);
		PointerSet_print(ps4);
		PointerSet* ps5 = PointerSet_difference(ps, ps2);
		PointerSet_print(ps5);
		PointerSet_union_inplace(ps5, ps3);
		PointerSet_print(ps5);
	}
	
	if(test_vec) {
		printf("vec testing nyi\n");
	}
	
	
	if(test_fs) {
		printf("fs testing nyi\n");
	}
	
	
	if(test_b_vs_t) {
		double start;
		double ttime = 1, btime = 0;
		int inc = 2;
		int max = 700;
		
		
		while(ttime > btime) {
// 			int i = 0;
			
// 			int64_t* nums = malloc(max * sizeof(*nums));
// 			for(i = 0; i < max; i++) nums[i] = frandNorm() * 1000000 + 1;
			
	// 		int64_tSet set;
	// 		int64_tSet_init(&set);
// 			PointerSet set;
// 			HashTable hash;
			
			
			start = getCurrentTimePerf();
// 			PointerSet_init(&set);
// 			for(i = 0; i < max; i++) {
	// 			int64_tSet_insert(&set, nums[i]);
// 				PointerSet_insert(&set, (void*)nums[i]);
				
// 			}
// 			PointerSet_destroy(&set);
			
			
			btime = timeSincePerf(start);
			printf("set time: %fms\n", btime * 1000);
			
			
			
			start = getCurrentTimePerf();
			
// 			for(i = 0; i < max; i++) {
// 				tsearch(&nums[i], &tree, intcomp);
// 			}
// 			tdestroy(tree, nothin);
			
			ttime = timeSincePerf(start);
			printf("tree time: %fms\n", ttime * 1000);
			
			
// 			max += inc;
		}
		
		printf("passed at: %d\n", max - inc);
	}
	
	
	if(test_commas) {
		iprintf("%+,d\n", 1);
		iprintf("%+,d\n", -12l);
		iprintf("%,d\n", -123l);
		iprintf("%,d\n", 1234);
		iprintf("%,d\n", 12345);
		iprintf("%,d\n", 123456);
		iprintf("%,d\n", 1234567);
		iprintf("%,d\n", 12345678);
		iprintf("%,d\n", 123456789);
		iprintf("%,d\n", 1234567890);
		iprintf("%+,d\n", 12345678901);
		iprintf("%,d\n", 123456789012);
		iprintf("%,d\n", 1234567890123);
		iprintf("%p\n", &c);
		
	}
	
	
	return 0;
}




char rand_char() {
	static char* chars = "QWERTYUIOPASDFGHJKLZXCVBNM";
	return chars[rand() % 26];
}

char** generate_random_strings(size_t len, size_t block_size) {
	char** out = malloc(len * block_size);
	int cl = 0;
	
	for(size_t i = 0; i < len; i++) {
		char* k = malloc(5);
	TRY_AGAIN:
		k[0] = rand_char();
		k[1] = rand_char();
		k[2] = rand_char();
		k[3] = rand_char();
		k[4] = 0;
		
		for(int j = 0; j < cl; j++) {
			if(0 == strcmp(k, out[j])) goto TRY_AGAIN;
		}
		
		*((char**)(out + (i * block_size))) = k;
	}
	
	return (char**)out;
}
