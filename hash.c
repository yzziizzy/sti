// Public Domain.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "hash_fns/MurmurHash3.h"
#include "hash.h"



#define MURMUR_SEED 718281828

 
static uint64_t hash_key(char* key, int64_t len);
static uint64_t hash_int64_key(uint64_t key);
 
 


#ifndef STI_HAS_STATIC_nextPOT
#define STI_HAS_STATIC_nextPOT
// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline static size_t nextPOT(size_t in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}

#endif







static ptrdiff_t oaht_find_bucket(struct HT_base_layout* ht, uint64_t hash, void* key) {
	int64_t startBucket, bi;
	
	bi = hash % ht->alloc_size; 
	startBucket = bi;
	
	struct bucket {
		uint64_t hash;
		void* key;
		char value[0];
	};
	
	do {
// 		struct hash_bucket* bucket;
		
		struct bucket* bucket = (struct bucket*)(ht->buckets + (bi * ht->stride));
		
		// empty bucket
		if(bucket->key == NULL) {
			return bi;
		}
		
		if(bucket->hash == hash) {
			switch(ht->key_mode) {
				case 's':
					if(!strcmp(key, bucket->key)) {
						return bi;
					}
					break;
					
				case 'p':
					if(!memcmp(key, bucket->key, ht->key_len)) {
						// bucket is the right one and contains a value already
						return bi;
					}
					break;
					
				case 'i':
					if(!memcmp(key, &bucket->key, ht->key_len)) {
						// bucket is the right one and contains a value already
						return bi;
					}
					break;
			}
				
			
			// collision, probe next bucket
		}
		
		bi = (bi + 1) % ht->alloc_size;
	} while(bi != startBucket);
	
	// should never reach here if the table is maintained properly
	assert(0);
	
	return -1;
}



// TODO: better return values and missing key handling
// returns 0 if val is set to the value
// return > 0  means the key was not found;
int oaht_getp_kptr(struct HT_base_layout* ht, void* key, void** valp) {
	uint64_t hash;
	int64_t bi;
	
	if(key == NULL) {
//		if(valp) *valp = NULL;
		return 2;
	}
	
	size_t key_len = ht->key_mode == 's' ? strlen(key) : ht->key_len;
	hash = hash_key(key, key_len);
	
	bi = oaht_find_bucket(ht, hash, key);
	if(bi < 0) {// || *(char**)(ht->buckets + (bi * ht->stride) + sizeof(uint64_t)) == NULL) {
//		*valp = NULL;
		return 1;
	}
	
	uint64_t bhash = *(uint64_t*)(ht->buckets + (bi * ht->stride));
	if(!bhash) return 1;
	
	size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
//	printf("oaht get - bi: %ld (alloc %ld, stride %ld)\n", bi, ht->alloc_size, ht->stride);
	
	//                         index             hash               key    
	*valp = ht->buckets + (bi * ht->stride) + sizeof(uint64_t) + key_width;
	memcpy(*valp, ht->buckets + (bi * ht->stride) + sizeof(uint64_t) + key_width, ht->stride - 8 - 8);
	return 0;
}

// zero for success
int oaht_get_klit(struct HT_base_layout* ht, uint64_t key, void* val) {
	return oaht_get_kptr(ht, &key, val);
}

int oaht_get_kptr(struct HT_base_layout* ht, void* key, void* val) {
	void* p = NULL;
	
	int ret = oaht_getp_kptr(ht, key, &p);
	if(ret == 0) {
		size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
		
		memcpy(
			val, 
			p,
			ht->stride - sizeof(uint64_t) - key_width
		);
	}
	
	return ret;
}


// zero for success
int oaht_set_kptr(struct HT_base_layout* ht, void* key, void* val) {
	uint64_t hash;
	int64_t bi;
	
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};
	
	// check size and grow if necessary
	if((float)ht->fill / (float)ht->alloc_size >= 0.75) {
		oaht_resize(ht, ht->alloc_size * 2);
	}
	
	if(ht->key_mode == 's') {
		hash = hash_key(key, strlen(key));
	}
	else { /* if(ht->key_mode == 'p') { */
		hash = hash_key(key, ht->key_len);
	} /*
	else {
		hash = hash_key((char*)&key, ht->key_len);
	} */
	
	bi = oaht_find_bucket(ht, hash, key);
	if(bi < 0) return 1;
	
// 	printf("oaht set - bi: %ld (alloc %ld, stride %ld)\n", bi, ht->alloc_size, ht->stride);

	void* b = ht->buckets + (ht->stride * bi);
	#define BK ((struct bucket*)b)
	
	if(BK->hash == 0) {
		// new bucket
		ht->fill++;
	}
	
	
	size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
	memcpy(b + sizeof(uint64_t) + key_width, val, ht->stride - sizeof(uint64_t) - key_width);
	
	if(ht->key_mode == 'i') {
		memcpy(&BK->key, key, ht->key_len);
	}
	else {
		BK->key = key;
	}
	
	
	BK->hash = hash;
#undef BK
	
	return 0;
}

// zero for success
int oaht_set_klit(struct HT_base_layout* ht, uint64_t key, void* val) {
	return oaht_set_kptr(ht, &key, val);
}

// should always be called with a power of two
int oaht_resize(struct HT_base_layout* ht, size_t newSize) {
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};
	
	char* old, *op;
	int64_t oldlen = ht->alloc_size;
	int64_t i, n, bi;
	
	old = op = ht->buckets;
	
	ht->alloc_size = newSize;
	ht->buckets = calloc(1, ht->stride * newSize);
	if(!ht->buckets) return 1;
	
	for(i = 0, n = 0; i < oldlen && n < (int64_t)ht->fill; i++) {
		#define OP ((struct bucket*)(op + (ht->stride * i))) 
		
		if(OP->hash == 0) {
			continue;
		}

		#define BK ((struct bucket*)(ht->buckets + (ht->stride * bi))) 
		
		size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
		
		bi = oaht_find_bucket(ht, OP->hash, ht->key_mode == 'i' ? (char*)&OP->key : (char*)OP->key);
		memcpy(BK, OP, ht->stride);
		
		n++;
	}
	
#undef BK
#undef OP

	free(old);
	
	return 0;
}



// zero for success
int oaht_delete(struct HT_base_layout* ht, char* key) {
	uint64_t hash;
	int64_t bi, empty_bi, nat_bi;
	
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};
	
	
	/* do this instead of the deletion algorithm
	// check size and shrink if necessary
	if(obj->fill / alloc_size <= obj->shrink_ratio) {
		HT_resize(obj, alloc_size > 32 ? alloc_size / 2 : 16);
		alloc_size = obj->alloc_size;
	}
	*/
	
	size_t key_len = ht->key_mode == 's' ? strlen(key) : ht->key_len;
	hash = hash_key(key, key_len);
	bi = oaht_find_bucket(ht, hash, key);
	
	// if there's a key, work until an empty bucket is found
	// check successive buckets for colliding keys
	//   walk forward until the furthest colliding key is found
	//   move it to the working bucket.
	//   

	#define BK ((struct bucket*)(ht->buckets + (ht->stride * bi))) 
	#define E_BK ((struct bucket*)(ht->buckets + (ht->stride * empty_bi))) 
	
	
	// nothing to delete, bail early
	if(BK->hash == 0) return 0;
	
	//
	empty_bi = bi;
	size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
	
	do {
		bi = (bi + 1) % ht->alloc_size;
		if(BK->hash == 0) {
			//empty bucket
			break;
		}
		
		// bucket the hash at the current index naturally would be in
		nat_bi = BK->hash % ht->alloc_size;
		
		if((bi > empty_bi && // after the start
			(nat_bi <= empty_bi /* in a sequence of probed misses */ || nat_bi > bi /* wrapped all the way around */)) 
			||
			(bi < empty_bi && // wrapped around
			(nat_bi <= empty_bi /* in a sequence of probed misses... */ && nat_bi > bi /* ..from before the wrap */))) {
			
			// move this one back
			memcpy(E_BK, BK, ht->stride);
			
			empty_bi = bi;
		}
	} while(1);
	
	E_BK->hash = 0;
	ht->fill--;
	
	return 0;
}




// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int oaht_nextp(struct HT_base_layout* ht, void** iter, void** key, void** valp) { 
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};

	#define B ((struct bucket*)b)

	void* b = *iter;
	
	// for starting the loop
	if(b == NULL) b = ht->buckets - ht->stride;
	
	// TODO: make sure strings and pointers and such are all reasonably handled for the key
	do {
		b += ht->stride;
		if(b >= ht->buckets + (ht->alloc_size * ht->stride)) {
			// end of the list
			*valp = NULL;
			*key = NULL;
			return 0;
		}
	} while(!B->key);
	
	*key = B->key;
	
	size_t key_width = ht->key_mode == 'i' ? ht->key_len : sizeof(char*);
	// TODO: implicit key width
	*valp = b + sizeof(uint64_t) + key_width;
	*iter = b;
	#undef B
	
	return 1;
}

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int oaht_next(struct HT_base_layout* ht, void** iter, void** key, void* val) { 
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};

	#define B ((struct bucket*)b)

	void* b = *iter;
	
	// a tiny bit of idiot-proofing
	if(b == NULL) b = ht->buckets - ht->stride;
	
	do {
		b += ht->stride;
		if(b >= ht->buckets + (ht->alloc_size * ht->stride)) {
			// end of the list
			*key = NULL;
			return 0;
		}
	} while(!B->key);
	
	*key = B->key;
	
	memcpy(val, &B->value, ht->stride - sizeof(uint64_t) - sizeof(char*)); // BUG: implicit key length
	*iter = b;
	#undef B
	
	return 1;
}












// uses a truncated 128bit murmur3 hash, except never returns 0
static uint64_t hash_key(char* key, int64_t len) {
	uint64_t hash[2];
	
	// len is optional
//	if(len <= 0) len = strlen(key);
	
	MurmurHash3_x64_128(key, len, MURMUR_SEED, hash);
	
	return hash[0] == 0 ? 1 : hash[0];
}

static uint64_t hash_int64_key(uint64_t key) {
	uint64_t hash[2];
	
	MurmurHash3_x64_128(&key, 8, MURMUR_SEED, hash);
	
	return hash[0] == 0 ? 1 : hash[0];
}

