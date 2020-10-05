#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


#include "../sti.h"
#include <time.h>



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


int rb_is_red(rb_node* n);


int check_double_red(rb_node* n) {
	if(!n) return 0;
	if(rb_is_red(n)) {
		if(n->kids[0] && rb_is_red(n->kids[0])) goto BAD;
		if(n->kids[1] && rb_is_red(n->kids[1])) goto BAD;
	}
	return check_double_red(n->kids[0]) + check_double_red(n->kids[1]);
BAD:
	printf("\n!!! ---- double red at %s -----\n\n", n->key);
	return 1;
}

int check_black_height(rb_node* n, int* bad) {
	if(!n) return 1;
	int a = check_black_height(n->kids[0], bad);
	int b = check_black_height(n->kids[1], bad);
	if(a != b) {
		printf("\n\n!!! --- height mismatch at %s --------------\n\n", n->key);
		*bad = 1;
	}
	return a + !rb_is_red(n);
}

char rand_char() {
	static char* chars = "QWERTYUIOPASDFGHJKLZXCVBNM";
	return chars[rand() % 26];
}

char** generate_random_keys(int len) {
	char** out = malloc(len * sizeof(*out));
	
	int cl = 0;
	
	for(int i = 0; i < len; i++) {
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
		
		out[i] = k;
	}
	
	return out;
}


long test_trav(char* key, void* data, void* user_data) {
	printf("%s\n", key);
	return 0;
}

int str_sort(const void* a, const void* b) {
	char* c = *((char**)a);
	char* d = *((char**)b);
	return strcmp(c, d);
}

void test_tree(int len) {
	RB(int) t;
	t.tree.root = NULL;
	int bad = 0;
	int val = 5;
	
	char** keys = generate_random_keys(len);
// 	double start, end;
	
	
	
	for(int i = 0; i < len; i++) {
// 		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\ninserting %s\n", keys[i]);
		RB_insert(&t, keys[i], val);
// 		html_print_node(t.tree.root, get_max_height(t.tree.root)); html_spacer();
		check_black_height(t.tree.root, &bad);
		if(bad) {
			printf("black height mismatch\n");
			fprintf(dbg, "black height mismatch\n");
			html_print_node(t.tree.root, get_max_height(t.tree.root)); html_spacer();
			html_footer();
			exit(1);
		}
		if(check_double_red(t.tree.root)) {
			printf("double red found\n");
			html_footer();
			exit(1);
		}
	}
	
	
	
	for(int i = 0; i < len; i++) {
// 		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\ndeleting %s\n", keys[i]);
// 		fprintf(dbg, "Deleted %s\n", keys[i]);
		RB_delete(&t, keys[i], NULL);
// 		printf("-------deletion complete\n");
// 		printf(".root: %p\n", t.tree.root);
// 		html_print_node(t.tree.root, get_max_height(t.tree.root)); html_spacer();
// 		fflush(dbg);
		check_black_height(t.tree.root, &bad);
		if(bad) {
			fprintf(dbg, "black height mismatch\n");
			html_print_node(t.tree.root, get_max_height(t.tree.root)); html_spacer();
			html_footer();
			
			printf("black height mismatch\n");
			exit(1);
		}
		if(check_double_red(t.tree.root)) {
			printf("double red found\n");
			html_footer();
			exit(1);
		}
	}
	
	RB_trunc(&t);
}



void hash_race(int len, int htinit) {
	double start, end;
	RB(int) t;
	RB_init(&t);
	int val = 5;
	
	HT(int) ht;
	HT_init(&ht, htinit);
	
	char** keys = generate_random_keys(len);
	
	printf("N = %d, hash initial size = %d\n", len, htinit);
	
	printf("Inserts:\n");
	start = getCurrentTime();
	for(int i = 0; i < len; i++) {
		RB_insert(&t, keys[i], val);
	}
	end = getCurrentTime();
	
	printf(" Red-Black Tree: %fms\n", (end - start)* 1000);
	
	start = getCurrentTime();
	for(int i = 0; i < len; i++) {
		HT_set(&ht, keys[i], val);
	}
	end = getCurrentTime();
	
	printf(" Hash Table:     %fms\n", (end - start)* 1000);
	
	//---------------------------------
	
	printf("Finds:\n");
	start = getCurrentTime();
	for(int i = 0; i < len; i++) {
		RB_find(&t, keys[i], NULL);
	}
	end = getCurrentTime();
	
	printf(" Red-Black Tree: %fms\n", (end - start)* 1000);
	
	start = getCurrentTime();
	for(int i = 0; i < len; i++) {
		HT_get(&ht, keys[i], &val);
	}
	end = getCurrentTime();
	
	printf(" Hash Table:     %fms\n", (end - start)* 1000);
	
	//---------------------------------
	
	printf("Deletes:\n");
	start = getCurrentTime();
	for(int i = 0; i < len; i++) {
		RB_delete(&t, keys[i], NULL);
	}
	end = getCurrentTime();
	
	printf(" Red-Black Tree: %fms\n", (end - start)* 1000);
	
	start = getCurrentTime();
	for(int i = 0; i < len; i++) {
		HT_delete(&ht, keys[i]);
	}
	end = getCurrentTime();
	
	printf(" Hash Table:     %fms\n", (end - start)* 1000);
	
	printf("\n\n");
	
	
}


int main(int argc, char* argv[]) {
	
	html_header("./debug.html");
	
// 	test_tree(100000);
// 	hash_race(strtol(argv[1], NULL, 10), strtol(argv[2], NULL, 10) );
	
	hash_race(100, 128);
	hash_race(1000, 128);
	hash_race(10000, 128);
	hash_race(100000, 128);
	hash_race(1000000, 128);
	//*/
	
	/*
	rb_tree_ t;
	t.root = NULL;
	int val = 5;
	
	for(int i = 0; i < 12; i++) {
		rb_insert_(&t, Qkeys[i], &val);
		
		fprintf(f, "i: %d\n", i);
		html_print_node(t.root, get_max_height(t.root)); html_spacer();
		
		check_black_height(t.root);
		
		print_tree(t.root, 0);
		printf("\n\n");
	}
	rb_delete(&t, "V", NULL);
// 	rb_delete(&t, "N", NULL);
// 	rb_delete(&t, "Ra", NULL);
	
 	html_print_node(t.root, get_max_height(t.root)); html_spacer();
	fprintf(f, "Deleting\n");
	printf("----------\n");
	
	rb_delete(&t, "Z", NULL);
 	html_print_node(t.root, get_max_height(t.root)); html_spacer();
	*/
	html_footer();
	
	
// 	printf("depth: %d\n", get_depth(t.root));
// 	print_level(t.root, 0, 0); printf("\n");
// 	print_level_lines(t.root, 0, 0); printf("\n");
// 	print_level(t.root, 1, 0); printf("\n");
// 	print_level_lines(t.root, 1, 0); printf("\n");
// 	print_level(t.root, 2, 0); printf("\n");
// 	print_level_lines(t.root, 2, 0);  printf("\n");
// 	print_level(t.root, 3, 0); printf("\n");
// // 	print_level_lines(t.root, 3, 0);
// 	print_level(t.root, 4, 0);
	
	printf("\n\n");
	
	return 0;
}



