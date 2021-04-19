#ifndef __sti__hash_h__
#define __sti__hash_h__

// Public Domain.

#include <stdint.h>
#include <stddef.h>


// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline static size_t HT_nextPOT(size_t in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}


// shrink_ratio: set greater than 1.0 to entirely disable, default 99.0
#define HT(Type) \
struct { \
	size_t alloc_size; \
	union { \
		size_t fill; \
		Type* dummyPtr; \
	}; \
	struct { \
		uint64_t hash; \
		char* key; \
		Type value; \
	} *buckets; \
}

// this must be calculated manually because compilers might 
//   add padding at the end of the struct for small Types
#define HT_STRIDE(h) (sizeof((h)->buckets->hash) + sizeof((h)->buckets->key) + sizeof((h)->buckets->value))

#define HT_init(h, sz) \
do { \
	(h)->alloc_size = HT_nextPOT(sz);\
	(h)->fill = 0;\
	(h)->buckets = calloc(1, HT_STRIDE(h) * (h)->alloc_size);\
} while(0)

#define HT_destroy(h) \
do { \
	(h)->alloc_size = 0; \
	(h)->fill = 0; \
	if((h)->buckets) free((h)->buckets); \
} while(0)



int oaht_resize(char** buckets, size_t stride, size_t* fill, size_t* alloc_size, size_t newSize);


// returns 0 if val is set to the value
// *val == NULL && return > 0  means the key was not found;
int oaht_getp(char* buckets, size_t stride, size_t alloc_size, char* key, char** valp);
int oaht_get(char* buckets, size_t stride, size_t alloc_size, char* key, char* val);

#define HT_getp(h, key, valp) oaht_getp((char*)(h)->buckets, HT_STRIDE(h), (h)->alloc_size, key, (char**)(1 ? valp : &((h)->dummyPtr)))
#define HT_get(h, key, valp) oaht_get((char*)(h)->buckets, HT_STRIDE(h), (h)->alloc_size, key, (char*)(1 ? valp : &((h)->buckets->value)))

int oaht_set(char** buckets, size_t stride, size_t* fill, size_t* alloc_size, char* key, char* val);
#define HT_set(h, key, val) oaht_set((char**)&((h)->buckets), HT_STRIDE(h), &(h)->fill, &(h)->alloc_size, key, (char*)(1 ? &(val) : &((h)->buckets->value)))

int oaht_delete(char** buckets, size_t stride, size_t* fill, size_t* alloc_size, char* key);
#define HT_delete(h, key) oaht_delete((char**)&((h)->buckets), HT_STRIDE(h), &(h)->fill, &(h)->alloc_size, key)

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int oaht_nextp(char* buckets, size_t stride, size_t alloc_size, void** iter, char** key, char** valp);
#define HT_nextp(h, iter, keyp, valp) oaht_nextp((char*)(h)->buckets, HT_STRIDE(h), (h)->alloc_size, iter, keyp, (char**)(1 ? valp : &((h)->dummyPtr)))

int oaht_next(char* buckets, size_t stride, size_t alloc_size, void** iter, char** key, char* val);
#define HT_next(h, iter, keyp, valp) oaht_next((char*)(h)->buckets, HT_STRIDE(h), (h)->alloc_size, iter, keyp, (char*)(1 ? valp : &((h)->buckets->value)))



/*
Loop macro magic

https://www.chiark.greenend.org.uk/~sgtatham/mp/

HashTable obj;
HT_LOOP(&obj, key, char*, val) {
	printf("loop: %s, %s", key, val);
}

effective source:

	#define HT_LOOP(obj, keyname, valtype, valname)
	if(0)
		finished: ;
	else
		for(char* keyname;;) // internal declarations, multiple loops to avoid comma op funny business
		for(valtype valname;;)
		for(void* iter = NULL ;;)
			if(HT_next(obj, iter, &keyname, &valname))
				goto main_loop;
			else
				while(1)
					if(1) {
						// when the user uses break
						goto finished;
					}
					else
						while(1)
							if(!HT_next(obj, iter, &keyname, &valname)) {
								// normal termination
								goto finished;
							}
							else
								main_loop:
								//	{ user block; not in macro }
*/
#define HASH__PASTEINNER(a, b) a ## b
#define HASH__PASTE(a, b) HASH__PASTEINNER(a, b) 
#define HASH__ITER(key, val) HASH__PASTE(hashtable_iter_ ## key ## __ ## val ## __, __LINE__)
#define HASH__FINISHED(key, val) HASH__PASTE(hashtable_finished__ ## key ## __ ## val ## __, __LINE__)
#define HASH__MAINLOOP(key, val) HASH__PASTE(hashtable_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define HT_EACH(obj, keyname, valtype, valname) \
if(0) \
	HASH__FINISHED(key, val): ; \
else \
	for(char* keyname ;;) \
	for(valtype valname ;;) \
	for(void* HASH__ITER(key, val) = NULL ;;) \
		if(HT_next(obj, & (HASH__ITER(key, val)), &keyname, &valname)) \
			goto HASH__MAINLOOP(key, val); \
		else \
			while(1) \
				if(1) { \
					goto HASH__FINISHED(key, val); \
				} \
				else \
					while(1) \
						if(!HT_next(obj, & (HASH__ITER(key, val)), &keyname, &valname)) { \
							goto HASH__FINISHED(key, val); \
						} \
						else \
							HASH__MAINLOOP(key, val) :
							
							//	{ user block; not in macro }


#define HT_EACHP(obj, keyname, valtype, valname) \
if(0) \
	HASH__FINISHED(key, val): ; \
else \
	for(char* keyname ;;) \
	for(valtype* valname ;;) \
	for(void* HASH__ITER(key, val) = NULL ;;) \
		if(HT_nextp(obj, & (HASH__ITER(key, val)), &keyname, &valname)) \
			goto HASH__MAINLOOP(key, val); \
		else \
			while(1) \
				if(1) { \
					goto HASH__FINISHED(key, val); \
				} \
				else \
					while(1) \
						if(!HT_nextp(obj, & (HASH__ITER(key, val)), &keyname, &valname)) { \
							goto HASH__FINISHED(key, val); \
						} \
						else \
							HASH__MAINLOOP(key, val) :
							
							//	{ user block; not in macro }
//










struct pointer_hash_bucket {
	uint64_t hash;
	char* key;
	void* value;
};


typedef struct pointer_hash_table {
	size_t alloc_size;
	size_t fill;
	float grow_ratio; // default 0.75
	float shrink_ratio; // set greater than 1.0 to entirely disable, default 99.0
	struct pointer_hash_bucket* buckets; 
} PointerHashTable;

#define PointerHashTable(x) struct hash_table

// NOTE: if you pass in garbage pointers you deserve the segfault

PointerHashTable* PHT_create(int allocPOT);
int PHT_init(PointerHashTable* obj, int allocPOT);
void PHT_destroy(PointerHashTable* obj, int free_values_too);
int PHT_resize(PointerHashTable* obj, int newSize);

// returns 0 if val is set to the value
// *val == NULL && return > 0  means the key was not found;
int PHT_get(PointerHashTable* obj, char* key, void** val);
int PHT_getInt(PointerHashTable* obj, char* key, int64_t* val);

// zero for success
// key's memory is not managed internally. strdup it yourself
int PHT_set(PointerHashTable* obj, char* key, void* val);
int PHT_setInt(PointerHashTable* obj, char* key, int64_t val);

int PHT_delete(PointerHashTable* obj, char* key);

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int PHT_next(PointerHashTable* obj, void** iter, char** key, void** value);

/*
Loop macro magic

https://www.chiark.greenend.org.uk/~sgtatham/mp/

PointerHashTable obj;
HT_EACH(&obj, key, char*, val) {
	printf("loop: %s, %s", key, val);
}

effective source:

	#define HT_EACh(obj, keyname, valtype, valname)
	if(0)
		finished: ;
	else
		for(char* keyname;;) // internal declarations, multiple loops to avoid comma op funny business
		for(valtype valname;;)
		for(void* iter = NULL ;;)
			if(PHT_next(obj, iter, &keyname, &valname))
				goto main_loop;
			else
				while(1)
					if(1) {
						// when the user uses break
						goto finished;
					}
					else
						while(1)
							if(!PHT_next(obj, iter, &keyname, &valname)) {
								// normal termination
								goto finished;
							}
							else
								main_loop:
								//	{ user block; not in macro }
*/
#define PHASH__PASTEINNER(a, b) a ## b
#define PHASH__PASTE(a, b) PHASH__PASTEINNER(a, b) 
#define PHASH__ITER(key, val) PHASH__PASTE(hashtable_iter_ ## key ## __ ## val ## __, __LINE__)
#define PHASH__FINISHED(key, val) PHASH__PASTE(hashtable_finished__ ## key ## __ ## val ## __, __LINE__)
#define PHASH__MAINLOOP(key, val) PHASH__PASTE(hashtable_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define PHT_EACH(obj, keyname, valtype, valname) \
if(0) \
	PHASH__FINISHED(key, val): ; \
else \
	for(char* keyname ;;) \
	for(valtype valname ;;) \
	for(void* PHASH__ITER(key, val) = NULL ;;) \
		if(PHT_next(obj, & (PHASH__ITER(key, val)), &keyname, (void**)&valname)) \
			goto PHASH__MAINLOOP(key, val); \
		else \
			while(1) \
				if(1) { \
					goto PHASH__FINISHED(key, val); \
				} \
				else \
					while(1) \
						if(!PHT_next(obj, & (PHASH__ITER(key, val)), &keyname, (void**)&valname)) { \
							goto PHASH__FINISHED(key, val); \
						} \
						else \
							PHASH__MAINLOOP(key, val) :
							
							//	{ user block; not in macro }





#endif // __sti__hash_h__
