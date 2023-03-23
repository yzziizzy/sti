#include "../hash.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#define ERROR(a, ...) printf("%s:%d  " a "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)




void print_hash_table(struct HT_base_layout* ht) {
	
	for(size_t i = 0; i < ht->alloc_size; i++) {
		char* b = (char*)ht->buckets + (i * ht->stride);
		
		size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
		
		uint64_t* b_hash = (uint64_t*)b;
		char* b_key = b + sizeof(uint64_t);
		char* b_val = b + sizeof(uint64_t) + key_width;;
		
		printf("bucket #%ld", i);
		
		if(*b_hash == 0) printf("  [empty]\n\n");
		else {
			
			printf("\n  hash: %lx\n", *b_hash);
			if(ht->key_mode == 's')
				printf("  key: '%s'\n", *(char**)b_key);
			else
				printf("  key: %lx\n", *b_key);
				
			printf("  val: %lx\n", *(int*)b_val);
			
			printf("  \n");
		}
	}
	
}



typedef struct {
	float a,b,c;
} vec3;

#define V(...) ((vec3){ __VA_ARGS__ })


int main() {
	int n;
	int nstrs = 10;
	char** strlist = malloc(sizeof(*strlist) * nstrs); 
	
	for(int i = 0; i < nstrs; i++) {
		char* b = malloc(20);
		sprintf(b, "%d", i);
		strlist[i] = b;
	}
	
	
	
	HT(int) str_int;
	HT_init(&str_int, 16);
/*	
	for(int i = 0; i < nstrs; i++) {
		HT_set(&str_int, strlist[i], i);
	}
	
	print_hash_table(&str_int.base);
	
	
	for(int i = 0; i < nstrs; i++) {
		int n = 0;
		if(HT_get(&str_int, strlist[i], &n)) ERROR("not found %d", i);
		if(n != i) ERROR("wrong value '%s' != %d", strlist[i], i); 
	}
	

	if(HT_delete(&str_int, "3")) ERROR("could not delete");

	n = 0;
	if(!HT_get(&str_int, "3", &n)) ERROR("wrongly found");
	if(n != 0) ERROR("wrong value"); 
*/
	char* one = strdup("one");
	char* two = strdup("two");
	char* three = strdup("three");
	
	// reset
	HT_destroy(&str_int);
	HT_init(&str_int, 16);

	HT_set(&str_int, one, 1);
	HT_set(&str_int, two, 2);
	HT_set(&str_int, three, 3);

	print_hash_table(&str_int.base);

	n = 0;
	if(HT_get(&str_int, one, &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&str_int, two, &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&str_int, three, &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 

	// make sure there's no weird pointer fuckery and the value itself is getting used as the key
	n = 0;
	if(HT_get(&str_int, "one", &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&str_int, "two", &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&str_int, "three", &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 

	
	HT(int, int) int_int;
	HT_init(&int_int, 16);
	
	HT_set(&int_int, 1, 1);
	HT_set(&int_int, 2, 2);
	HT_set(&int_int, 3, 3);

	n = 0;
	if(HT_get(&int_int, 1, &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&int_int, 2, &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&int_int, 3, &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 

	// reset
	HT_destroy(&int_int);
	HT_init(&int_int, 16);
	
	int n1 = 1, n2 = 2, n3 = 3;
	
	HT_set(&int_int, n1, 1);
	HT_set(&int_int, n2, 2);
	HT_set(&int_int, n3, 3);
	
	n = 0;
	if(HT_get(&int_int, n1, &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&int_int, n2, &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 0;
	if(HT_get(&int_int, n3, &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 
	
	
	HT(vec3, int) vec_int;
	HT_init(&vec_int, 16);
	
	HT_set(&vec_int, V(1,2,3), 1);
	HT_set(&vec_int, V(2,3,4), 2);
	HT_set(&vec_int, V(3,4,5), 3);
	
	n = 0;
	if(HT_get(&vec_int, V(1,2,3), &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 2;
	if(HT_get(&vec_int, V(2,3,4), &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 3;
	if(HT_get(&vec_int, V(3,4,5), &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 
	
	vec3 vn1 = V(1,2,3);
	vec3 vn2 = V(2,3,4);
	vec3 vn3 = V(3,4,5);
	
	// reset
	HT_destroy(&vec_int);
	HT_init(&vec_int, 16);
	
	HT_set(&vec_int, vn1, 1);
	HT_set(&vec_int, vn2, 2);
	HT_set(&vec_int, vn3, 3);
	
	n = 0;
	if(HT_get(&vec_int, vn1, &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 2;
	if(HT_get(&vec_int, vn2, &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 3;
	if(HT_get(&vec_int, vn3, &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 
	
	
	n = 0;
	if(HT_get(&vec_int, V(1,2,3), &n)) ERROR("not found");
	if(n != 1) ERROR("wrong value"); 
	
	n = 2;
	if(HT_get(&vec_int, V(2,3,4), &n)) ERROR("not found");
	if(n != 2) ERROR("wrong value"); 
	
	n = 3;
	if(HT_get(&vec_int, V(3,4,5), &n)) ERROR("not found");
	if(n != 3) ERROR("wrong value"); 
	
	
	
	
	return 0;
}








