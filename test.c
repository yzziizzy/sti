// Public Domain.

#define _GNU_SOURCE     /* Expose declaration of tdestroy() */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <math.h>
#include <search.h> // for other testing

#include <float.h> // for float limts of conversion testing


#include "sti.h"
#include "./string.h"


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
	
	
	while ((c = getopt (argc, argv, "csvf1piI")) != -1) {
		switch(c) {
			case 's': test_sets = 1; break;
			case 'v': test_vec = 1; break;
			case 'f': test_fs = 1; break;
			case '1': test_b_vs_t = 1; break;
			case 'p': test_rpn = 1; break;
			case 'i': test_iprintf = 1; break;
			case 'I': test_Iprintf = 1; break;
			case 'c': test_commas = 1; break;
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
