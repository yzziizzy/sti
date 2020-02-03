// Public Domain.

#define _GNU_SOURCE     /* Expose declaration of tdestroy() */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


#include <search.h> // for other testing


#include "sti.h"


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
	
	while ((c = getopt (argc, argv, "svf1p")) != -1) {
		switch(c) {
			case 's': test_sets = 1; break;
			case 'v': test_vec = 1; break;
			case 'f': test_fs = 1; break;
			case '1': test_b_vs_t = 1; break;
			case 'p': test_rpn = 1; break;
		}
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
	
	
	return 0;
}
