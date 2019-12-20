
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>



#include "sets.h"
#include "vec.h"
#include "fs.h"
#include "misc.h"




int main(int argc, char* argv[]) {
	char c;
	char test_sets = 0;
	char test_vec = 0;
	char test_fs = 0;
	
	while ((c = getopt (argc, argv, "s")) != -1) {
		switch(c) {
			case 's': test_sets = 1; break;
			case 'v': test_vec = 1; break;
			case 'f': test_fs = 1; break;
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
	
	
	return 0;
};
