// Public Domain.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "MurmurHash3.h"
#include "hash.h"



#define MURMUR_SEED 718281828

 
static uint64_t hash_key(char* key, int64_t len);
static ptrdiff_t find_bucket(PointerHashTable* obj, uint64_t hash, char* key);
 
 


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







static ptrdiff_t oaht_find_bucket(
	char* buckets, 
	size_t stride, 
	size_t alloc_size, 
	uint64_t hash, 
	char* key
) {
	int64_t startBucket, bi;
	
	bi = hash % alloc_size; 
	startBucket = bi;
	
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};
	
	do {
// 		struct hash_bucket* bucket;
		
		struct bucket* bucket = (struct bucket*)(buckets + (bi * stride));
		
		// empty bucket
		if(bucket->key == NULL) {
			return bi;
		}
		
		if(bucket->hash == hash) {
			if(!strcmp(key, bucket->key)) {
				// bucket is the right one and contains a value already
				return bi;
			}
			
			// collision, probe next bucket
		}
		
		bi = (bi + 1) % alloc_size;
	} while(bi != startBucket);
	
	// should never reach here if the table is maintained properly
	assert(0);
	
	return -1;
}



// TODO: better return values and missing key handling
// returns 0 if val is set to the value
// *val == NULL && return > 0  means the key was not found;
int oaht_getp(char* buckets, size_t stride, size_t alloc_size, char* key, char** valp) {
	uint64_t hash;
	int64_t bi;
	
	if(key == NULL) {
		if(valp) *valp = NULL;
		return 2;
	}
	
	hash = hash_key(key, -1);
	
	bi = oaht_find_bucket(buckets, stride, alloc_size, hash, key);
	if(bi < 0 || *(char**)(buckets + (bi * stride) + sizeof(uint64_t)) == NULL) {
		*valp = NULL;
		return 1;
	}
	
	//                 index             hash               key    
	*valp = buckets + (bi * stride) + sizeof(uint64_t) + sizeof(char*); 
	return 0;
}

int oaht_get(char* buckets, size_t stride, size_t alloc_size, char* key, char* val) {
	char* p = NULL;
	
	int ret = oaht_getp(buckets, stride, alloc_size, key, &p);
	if(ret == 0) {
		memcpy(
			val, 
			p,
			stride - sizeof(uint64_t) - sizeof(char*)
		);
	}
	
	return ret;
}



// zero for success
int oaht_set(char** buckets, size_t stride, size_t* fill, size_t* alloc_size, char* key, char* val) {
	uint64_t hash;
	int64_t bi;
	
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};
	
	// check size and grow if necessary
	if((float)*fill / (float)*alloc_size >= 0.75) {
		oaht_resize(buckets, stride, fill, alloc_size, *alloc_size * 2);
	}
	
	hash = hash_key(key, -1);
	
	bi = oaht_find_bucket(*buckets, stride, *alloc_size, hash, key);
	if(bi < 0) return 1;
	
// 	printf("oaht set - bi: %ld (alloc %ld, stride %ld)\n", bi, *alloc_size, stride);
	
#define BK ((struct bucket*)((*buckets) + (stride * bi))) 
	if(BK->key == NULL) {
		// new bucket
		(*fill)++;
	}
	
	memcpy(&BK->value, val, stride - sizeof(uint64_t) - sizeof(char*));
	BK->key = key;
	BK->hash = hash;
#undef BK
	
	return 0;
}




// should always be called with a power of two
int oaht_resize(char** buckets, size_t stride, size_t* fill, size_t* alloc_size, size_t newSize) {
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};
	
	char* old, *op;
	int64_t oldlen = *alloc_size;
	int64_t i, n, bi;
	
	old = op = *buckets;
	
	*alloc_size = newSize;
	*buckets = calloc(1, stride * newSize);
	if(!*buckets) return 1;
	
	for(i = 0, n = 0; i < oldlen && n < (int64_t)*fill; i++) {
		#define OP ((struct bucket*)(*op + (stride * i))) 
		
		if(OP->key == NULL) {
			continue;
		}

		#define BK ((struct bucket*)(*buckets + (stride * bi))) 
		
		bi = oaht_find_bucket(*buckets, stride, *alloc_size, OP->hash, OP->key);
		memcpy(&BK->value, &OP->value, stride - sizeof(uint64_t) - sizeof(char*));
		BK->hash = OP->hash;
		BK->key = OP->key;
		
		n++;
	}
	
#undef BK
#undef OP

	free(old);
	
	return 0;
}



// zero for success
int oaht_delete(char** buckets, size_t stride, size_t* fill, size_t* alloc_size, char* key) {
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
	hash = hash_key(key, -1);
	bi = oaht_find_bucket(*buckets, stride, *alloc_size, hash, key);
	
	// if there's a key, work until an empty bucket is found
	// check successive buckets for colliding keys
	//   walk forward until the furthest colliding key is found
	//   move it to the working bucket.
	//   

	#define BK ((struct bucket*)(*buckets + (stride * bi))) 
	#define E_BK ((struct bucket*)(*buckets + (stride * empty_bi))) 
	
	
	// nothing to delete, bail early
	if(BK->key == NULL) return 0;
	
	//
	empty_bi = bi;
	
	do {
		bi = (bi + 1) % (*alloc_size);
		if(BK->key == NULL) {
			//empty bucket
			break;
		}
		
		// bucket the hash at the current index naturally would be in
		nat_bi = BK->hash % *alloc_size;
		
		if((bi > empty_bi && // after the start
			(nat_bi <= empty_bi /* in a sequence of probed misses */ || nat_bi > bi /* wrapped all the way around */)) 
			||
			(bi < empty_bi && // wrapped around
			(nat_bi <= empty_bi /* in a sequence of probed misses... */ && nat_bi > bi /* ..from before the wrap */))) {
			
			// move this one back
			E_BK->key = BK->key;
			E_BK->hash = BK->hash;
			memcpy(&E_BK->value, &BK->value, stride - sizeof(uint64_t) - sizeof(char*));
			
			empty_bi = bi;
		}
	} while(1);
	
	E_BK->key = NULL;
	(*fill)--;
	
	return 0;
}




// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int oaht_nextp(char* buckets, size_t stride, size_t alloc_size, void** iter, char** key, char** valp) { 
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};

	#define B ((struct bucket*)b)

	char* b = *iter;
	
	// a tiny bit of idiot-proofing
	if(b == NULL) b = buckets - stride;
	
	do {
		b += stride;
		if(b >= buckets + (alloc_size * stride)) {
			// end of the list
			*valp = NULL;
			*key = NULL;
			return 0;
		}
	} while(!B->key);
	
	*key = B->key;
	*valp = &B->value;
	*iter = b;
	
	return 1;
}

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int oaht_next(char* buckets, size_t stride, size_t alloc_size, void** iter, char** key, char* val) { 
	struct bucket {
		uint64_t hash;
		char* key;
		char value;
	};

	#define B ((struct bucket*)b)

	char* b = *iter;
	
	// a tiny bit of idiot-proofing
	if(b == NULL) b = buckets - stride;
	
	do {
		b += stride;
		if(b >= buckets + (alloc_size * stride)) {
			// end of the list
			*key = NULL;
			return 0;
		}
	} while(!B->key);
	
	*key = B->key;
	memcpy(val, &B->value, stride - sizeof(uint64_t) - sizeof(char*));
	*iter = b;
	
	return 1;
}




















PointerHashTable* PHT_create(int allocPOT) {
	
	PointerHashTable* obj;
	
	obj = malloc(sizeof(*obj));
	if(!obj) return NULL;
	
	if(PHT_init(obj, allocPOT)) {
		fprintf(stderr, "Failed to initialized hash table\n");
		free(obj);
		return NULL;
	}
	
	return obj;
}


int PHT_init(PointerHashTable* obj, int minAllocSize) {
	size_t pot;
	
	pot = nextPOT(minAllocSize);
	pot = pot < 16 ? 16 : pot;
	
	obj->fill = 0;
	obj->alloc_size = pot;
	obj->grow_ratio = 0.75f;
	obj->shrink_ratio = 99.0f; // set greater than 1.0 to entirely disable
	obj->buckets = calloc(1, sizeof(*obj->buckets) * obj->alloc_size);
	if(!obj->buckets) {
		return 1;
	}
	
	return 0;
}


void PHT_destroy(PointerHashTable* obj, int free_values_too) {
	int64_t i, n;
	
	if(free_values_too) {
		for(i = 0, n = 0; i < (int64_t)obj->alloc_size && n < (int64_t)obj->fill; i++) {
			// only free valid pointers that also have a key
			// deleted items are assumed to be cleaned up by the user
			if(obj->buckets[i].key) {
				if(obj->buckets[i].value) free(obj->buckets[i].value);
				n++;
			}
		}
	}
	
	if(obj->buckets) free(obj->buckets);
//	free(obj); owner has to clean up
}



// uses a truncated 128bit murmur3 hash
static uint64_t hash_key(char* key, int64_t len) {
	uint64_t hash[2];
	
	// len is optional
	if(len <= 0) len = strlen(key);
	
	MurmurHash3_x64_128(key, len, MURMUR_SEED, hash);
	
	return hash[0];
}

static ptrdiff_t find_bucket(PointerHashTable* obj, uint64_t hash, char* key) {
	int64_t startBucket, bi;
	
	bi = startBucket = hash % obj->alloc_size; 
	
	do {
		struct pointer_hash_bucket* bucket;
		
		bucket = &obj->buckets[bi];
		
		// empty bucket
		if(bucket->key == NULL) {
			return bi;
		}
		
		if(bucket->hash == hash) {
			if(!strcmp(key, bucket->key)) {
				// bucket is the right one and contains a value already
				return bi;
			}
			
			// collision, probe next bucket
		}
		
		bi = (bi + 1) % obj->alloc_size;
	} while(bi != startBucket);
	
	// should never reach here if the table is maintained properly
	assert(0);
	
	return -1;
}






// should always be called with a power of two
int PHT_resize(PointerHashTable* obj, int newSize) {
	struct pointer_hash_bucket* old, *op;
	int64_t oldlen = obj->alloc_size;
	int64_t i, n, bi;
	
	old = op = obj->buckets;
	
	obj->alloc_size = newSize;
	obj->buckets = calloc(1, sizeof(*obj->buckets) * newSize);
	if(!obj->buckets) return 1;
	
	for(i = 0, n = 0; i < oldlen && n < (int64_t)obj->fill; i++) {
		if(op->key == NULL) {
			op++;
			continue;
		}
		
		bi = find_bucket(obj, op->hash, op->key);
		obj->buckets[bi].value = op->value;
		obj->buckets[bi].hash = op->hash;
		obj->buckets[bi].key = op->key;
		
		n++;
		op++;
	}
	
	free(old);
	
	return 0;
}

// TODO: better return values and missing key handling
// returns 0 if val is set to the value
// *val == NULL && return > 0  means the key was not found;
int PHT_get(PointerHashTable* obj, char* key, void** val) {
	uint64_t hash;
	int64_t bi;
	
	if(key == NULL) {
		if(val) *val = NULL;
		return 2;
	}
	
	hash = hash_key(key, -1);
	
	bi = find_bucket(obj, hash, key);
	if(bi < 0 || obj->buckets[bi].key == NULL) {
		*val = NULL;
		return 1;
	}
	
	*val = obj->buckets[bi].value; 
	return 0;
}

int PHT_getInt(PointerHashTable* obj, char* key, int64_t* val) {
	return PHT_get(obj, key, (void**)val);
} 


// zero for success
int PHT_set(PointerHashTable* obj, char* key, void* val) {
	uint64_t hash;
	int64_t bi;
	
	// check size and grow if necessary
	if(obj->fill / obj->alloc_size >= obj->grow_ratio) {
		PHT_resize(obj, obj->alloc_size * 2);
	}
	
	hash = hash_key(key, -1);
	
	bi = find_bucket(obj, hash, key);
	if(bi < 0) return 1;
	
	if(obj->buckets[bi].key == NULL) {
		// new bucket
// 		obj->buckets[bi].key = key;
// 		obj->buckets[bi].hash = hash;
		obj->fill++;
	}
	
	obj->buckets[bi].value = val;
	obj->buckets[bi].key = key;
	obj->buckets[bi].hash = hash;
	
	return 0;
}
// zero for success
int PHT_setInt(PointerHashTable* obj, char* key, int64_t val) {
	return PHT_set(obj, key, (void*)val);
}

// zero for success
int PHT_delete(PointerHashTable* obj, char* key) {
	uint64_t hash;
	int64_t bi, empty_bi, nat_bi;
	
	size_t alloc_size = obj->alloc_size;
	
	/* do this instead of the deletion algorithm
	// check size and shrink if necessary
	if(obj->fill / alloc_size <= obj->shrink_ratio) {
		PHT_resize(obj, alloc_size > 32 ? alloc_size / 2 : 16);
		alloc_size = obj->alloc_size;
	}
	*/
	hash = hash_key(key, -1);
	bi = find_bucket(obj, hash, key);
	
	// if there's a key, work until an empty bucket is found
	// check successive buckets for colliding keys
	//   walk forward until the furthest colliding key is found
	//   move it to the working bucket.
	//   
	
	// nothing to delete, bail early
	if(obj->buckets[bi].key == NULL) return 0;
	
	//
	empty_bi = bi;
	
	do {
		bi = (bi + 1) % alloc_size;
		if(obj->buckets[bi].key == NULL) {
			//empty bucket
			break;
		}
		
		// bucket the hash at the current index naturally would be in
		nat_bi = obj->buckets[bi].hash % alloc_size;
		
		if((bi > empty_bi && // after the start
			(nat_bi <= empty_bi /* in a sequence of probed misses */ || nat_bi > bi /* wrapped all the way around */)) 
			||
			(bi < empty_bi && // wrapped around
			(nat_bi <= empty_bi /* in a sequence of probed misses... */ && nat_bi > bi /* ..from before the wrap */))) {
			
			// move this one back
			obj->buckets[empty_bi].key = obj->buckets[bi].key;
			obj->buckets[empty_bi].hash = obj->buckets[bi].hash;
			obj->buckets[empty_bi].value = obj->buckets[bi].value;
			
			empty_bi = bi;
		}
	} while(1);
	
	obj->buckets[empty_bi].key = NULL;
	obj->fill--;
	
	return 0;
}

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int PHT_next(PointerHashTable* obj, void** iter, char** key, void** value) { 
	struct pointer_hash_bucket* b = *iter;
	
	// a tiny bit of idiot-proofing
	if(b == NULL) b = &obj->buckets[-1];
	
	do {
		b++;
		if(b >= obj->buckets + obj->alloc_size) {
			// end of the list
			*value = NULL;
			*key = NULL;
			return 0;
		}
	} while(!b->key);
	
	*key = b->key;
	*value = b->value;
	*iter = b;
	
	return 1;
}

