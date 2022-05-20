
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define MURMUR_SEED 718281828


#include "hash.h"
#include "hash_fns/MurmurHash3.h"

#include "string_int.h"






typedef struct memory_chunk {
	char* data;
	size_t len;
	size_t fill;
} memory_chunk_t;


typedef struct hash_bucket {
	uint64_t hash;
	char* value;
} hash_bucket_t;


typedef struct string_internment_table {
	
	struct {
		hash_bucket_t* buckets;
		size_t alloc_size;
		size_t fill;
	} ht;
	
	int pool_alloc;
	int pool_fill;
	memory_chunk_t* pool;
//	memory_chunk_t* fill_head;
	
} string_internment_table_t;

struct string_internment_table* global_string_internment_table;


static ptrdiff_t find_bucket_strint(string_internment_table_t* tab, uint64_t hash, char* key);
static ptrdiff_t find_bucket_n_strint(string_internment_table_t* tab, uint64_t hash, char* key, size_t key_len);
static int resize(string_internment_table_t* tab, size_t new_size);




void string_internment_table_init(struct string_internment_table** ptab) {
	struct string_internment_table* tab = calloc(1, sizeof(*tab));
	*ptab = tab;
	
	tab->pool_alloc = 32;
	tab->pool_fill = 0;
	tab->pool = malloc(tab->pool_alloc * sizeof(*tab->pool));
	
	
	tab->ht.alloc_size = 1024;
	tab->ht.fill = 0;
	tab->ht.buckets = calloc(1, tab->ht.alloc_size * sizeof(*tab->ht.buckets));
}




char* intern_string(struct string_internment_table* tab, char* s, int64_t len) {
	char* slot;

	
	memory_chunk_t* p = tab->pool;
	
	for(int i = 0; i < tab->pool_fill; i++, p++) {
		
		if(p->len - p->fill > len) {
			// found a spot;
			slot = p->data + p->fill;
			memcpy(slot, s, len);
			slot[len] = 0;
			
			p->fill += len + 1;
			
			return slot;
		}
		
	}
	
	//none of the chunks have enough room. add a new chunk.
	if(tab->pool_fill >= tab->pool_alloc) {
		tab->pool_alloc *= 2;
		tab->pool = realloc(tab->pool, tab->pool_alloc * sizeof(*tab->pool));
	}
	
	p = &tab->pool[tab->pool_fill];
	p->len = 1024 * 1024 * 8;
	p->fill = 0;
	p->data = malloc(p->len * sizeof(*p->data));
	
	tab->pool_fill++;
	
	slot = p->data + p->fill;
	memcpy(slot, s, len);	
	slot[len] = 0;
	
	p->fill += len + 1;
	
	return slot;
}



// returns a pointer to the permanent unique string
char* strint_(struct string_internment_table* tab, char* s) {
// returns a pointer to the permanent unique string
	return strnint_(tab, s, strlen(s));
}


// returns a pointer to the permanent unique string
char* strnint_(struct string_internment_table* tab, char* s, size_t slen) {
	uint64_t hash[2];
	int64_t bi;
	char* ps;
	
	MurmurHash3_x64_128(s, slen, MURMUR_SEED, hash);
	
	bi = find_bucket_n_strint(tab, hash[0], s, slen);
	
	if(tab->ht.buckets[bi].value) {
		// found it already, bail early
		return tab->ht.buckets[bi].value;
	}
		
	// add the string into the table
		
	// check size and grow if necessary
	if((float)tab->ht.fill / (float)tab->ht.alloc_size >= 0.75) {
		resize(tab, tab->ht.alloc_size * 2);
		bi = find_bucket_n_strint(tab, hash[0], s, slen);
	}
	
	ps = intern_string(tab, s, slen);
	
	tab->ht.buckets[bi].hash = hash[0];
	tab->ht.buckets[bi].value = ps;
	
	return ps;
}



// should always be called with a power of two
static int resize(string_internment_table_t* tab, size_t new_size) {
	hash_bucket_t* buckets, *old, *op;
	int64_t old_len = tab->ht.alloc_size;
	int64_t i, n, bi;
	
	old = tab->ht.buckets;
	
	buckets = calloc(1, new_size * sizeof(*buckets));
	
	for(i = 0, n = 0; i < old_len && n < tab->ht.fill; i++) {
		
		op = &old[n];
		
		if(op->value == NULL) {
			continue;
		}
		
		bi = find_bucket_strint(tab, op->hash, op->value);
		buckets[bi] = *op;
		
		n++;
	}
	
	tab->ht.buckets = buckets;
	tab->ht.alloc_size = new_size;

	free(old);
	
	return 0;
}






static ptrdiff_t find_bucket_strint(string_internment_table_t* tab, uint64_t hash, char* key) {
	int64_t startBucket, bi;
	
	bi = hash % tab->ht.alloc_size; 
	startBucket = bi;
	
	do {
		
		hash_bucket_t* bucket = tab->ht.buckets + bi;
		
		// empty bucket
		if(bucket->value == NULL) {
			return bi;
		}
		
		if(bucket->hash == hash) {
			if(!strcmp(key, bucket->value)) {
				// bucket is the right one and contains a value already
				return bi;
			}
			
			// collision, probe next bucket
		}
		
		bi = (bi + 1) % tab->ht.alloc_size;
	} while(bi != startBucket);
	
	// should never reach here if the table is maintained properly
	
	
	return -1;
}


static ptrdiff_t find_bucket_n_strint(string_internment_table_t* tab, uint64_t hash, char* key, size_t key_len) {
	int64_t startBucket, bi;
	
	bi = hash % tab->ht.alloc_size; 
	startBucket = bi;
	
	do {
		
		hash_bucket_t* bucket = tab->ht.buckets + bi;
		
		// empty bucket
		if(bucket->value == NULL) {
			return bi;
		}
		
		if(bucket->hash == hash) {
			if(!strncmp(key, bucket->value, key_len)) {
				// bucket is the right one and contains a value already
				return bi;
			}
			
			// collision, probe next bucket
		}
		
		bi = (bi + 1) % tab->ht.alloc_size;
	} while(bi != startBucket);
	
	// should never reach here if the table is maintained properly
	
	
	return -1;
}

