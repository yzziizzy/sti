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


struct HT_Pointer_Type {}; // key's length is stated explicitly
struct HT_Sizeof_Type {}; // key's length is determined by sizeof()
struct HT_String_Type {}; // null terminated key pointer

struct HT_Option_None {};
struct HT_Option_KeyLiteral {}; // keys are literal 64 bit values, not pointers to memory


// shrink_ratio: set greater than 1.0 to entirely disable, default 99.0

#define HT_ARG_N(_1, _2, _3, _4, _5, _6, N, ...) N
#define HT_RSEQ_N() 6, 5, 4, 3, 2, 1, 0
#define HT_NARG_(...) HT_ARG_N(__VA_ARGS__)
#define HT_NARG(...)  HT_NARG_(__VA_ARGS__, HT_RSEQ_N())

#define HT_CAT(a, b) a ## b

#define HT(...) HT_(HT_NARG(__VA_ARGS__), __VA_ARGS__)
#define HT_(n, ...) HT_CAT(HT_,n)(__VA_ARGS__)

// a single argument implies string keys and default options
#define HT_1(ValType) HT_STR(ValType)

// two arguments implies a sizeof()-able literal key type and default options 
#define HT_2(KeyType, ValType) HT_SZOF(KeyType, ValType) 

struct HT_base_layout {
	size_t alloc_size;
	size_t fill;
	size_t stride; // storing it once is less memory than passing it in every function call
	void* buckets;	
	uint32_t key_len; // keys longer than 4GB are not supported. 
//	uint16_t key_width; // number of bytes the key 
	uint8_t key_mode; // 's' = C string, 'p' = pointer to memory[key_len], 'i' = inline key sizeof() == key_len
};

#define HT_STR(ValType) \
struct { \
	struct HT_base_layout base; \
	struct { \
		struct HT_String_Type keyTypeFlag; \
		ValType valType; \
		ValType* valTypep; \
		char* keyType; \
	} meta[0]; \
}

#define HT_SZOF(KeyType, ValType) \
struct { \
	struct HT_base_layout base; \
	struct { \
		struct HT_Sizeof_Type keyTypeFlag; \
		ValType valType; \
		ValType* valTypep; \
		KeyType keyType; \
		\
		\
	} meta[0]; \
}


// this must be calculated manually because compilers might 
//   add padding at the end of the struct for small Types
#define HT_STRIDE(h) (sizeof(uint64_t) + sizeof((h)->meta[0].keyType) + sizeof((h)->meta[0].valType))

#define HT_KEYMODE(h) _Generic((h)->meta[0].keyTypeFlag, \
	struct HT_String_Type: 's', \
	struct HT_Sizeof_Type: 'i', \
	struct HT_Pointer_Type: 'p' \
)

#define HT_KEYMODE_LEN(h) _Generic((h)->meta[0].keyTypeFlag, \
	struct HT_String_Type: 0, \
	struct HT_Sizeof_Type: sizeof((h)->meta[0].valType), \
	struct HT_Pointer_Type: sizeof((h)->meta[0].valType) \
)

#define HT_KEY_WIDTH(h) _Generic((h)->meta[0].keyTypeFlag, \
	struct HT_String_Type: sizeof(char*), \
	struct HT_Sizeof_Type: sizeof((h)->meta[0].valType), \
	struct HT_Pointer_Type: sizeof(void*) \
)


#define HT_init(h, sz) \
do { \
	(h)->base.alloc_size = HT_nextPOT(sz); \
	(h)->base.fill = 0; \
	(h)->base.key_len = HT_KEYMODE_LEN(h); \
	(h)->base.key_mode = HT_KEYMODE(h); \
	(h)->base.stride =  sizeof(uint64_t) + HT_KEY_WIDTH(h) + sizeof((h)->meta[0].valType); \
	(h)->base.buckets = calloc(1, (h)->base.stride * (h)->base.alloc_size); \
} while(0)

#define HT_destroy(h) \
do { \
	(h)->base.alloc_size = 0; \
	(h)->base.fill = 0; \
	if((h)->base.buckets) free((h)->base.buckets); \
} while(0)



int oaht_resize(struct HT_base_layout* ht, size_t newSize);


// returns 0 if val is set to the value
// *val == NULL && return > 0  means the key was not found;
int oaht_getp_kptr(struct HT_base_layout* ht, void* key, void** valp);
int oaht_get_kptr(struct HT_base_layout* ht, void* key, void* val);

#define HT_TYPECHECK(h, a, b) (void*)(1 ? a : (h)->meta[0].b)

#define HT_get(h, key, valp) _Generic((h)->meta[0].keyTypeFlag, \
	struct HT_String_Type: oaht_get_kptr(&(h)->base, HT_TYPECHECK(h, key, keyType), HT_TYPECHECK(h, valp, valTypep)), \
	struct HT_Sizeof_Type: oaht_get_kptr(&(h)->base, HT_TYPECHECK(h, key, keyType), HT_TYPECHECK(h, valp, valTypep)) \
)

#define HT_getp(h, key, valp) _Generic((h)->meta[0].keyTypeFlag, \
	struct HT_String_Type: oaht_getp_kptr(&(h)->base, HT_TYPECHECK(h, key, keyType), (void**)(1 ? valp : &((h)->meta[0].valTypep))), \
	struct HT_Sizeof_Type: oaht_getp_kptr(&(h)->base, HT_TYPECHECK(h, key, keyType), (void**)(1 ? valp : &((h)->meta[0].valTypep))) \
)

int oaht_set_kptr(struct HT_base_layout* ht, void* key, void* val);
#define HT_set(h, key, val) _Generic((h)->meta[0].keyTypeFlag, \
	struct HT_String_Type: oaht_set_kptr(&(h)->base, HT_TYPECHECK(h, key, keyType), HT_TYPECHECK(h, val, valTypep)), \
	struct HT_Sizeof_Type: oaht_set_kptr(&(h)->base, HT_TYPECHECK(h, key, keyType), HT_TYPECHECK(h, val, valTypep)) \
)



//#define HT_set(h, key, val) oaht_set((char**)&((h)->buckets), HT_STRIDE(h), &(h)->fill, &(h)->alloc_size, key, (char*)(1 ? &(val) : &((h)->buckets->value)))

int oaht_delete(struct HT_base_layout* ht, char* key);
#define HT_delete(h, key) oaht_delete(&(h)->base, key)

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int oaht_nextp(struct HT_base_layout* ht, void** iter, char** key, char** valp);
#define HT_nextp(h, iter, keyp, valp) oaht_nextp((char*)(h)->buckets, HT_STRIDE(h), (h)->alloc_size, iter, keyp, (char**)(1 ? valp : &((h)->dummyPtr)))

int oaht_next(struct HT_base_layout* ht, void** iter, char** key, char* val);
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
	HASH__FINISHED(keyname, val): ; \
else \
	for(char* keyname ;;) \
	for(valtype valname ;;) \
	for(void* HASH__ITER(keyname, val) = NULL ;;) \
		if(HT_next(obj, & (HASH__ITER(keyname, val)), &keyname, &valname)) \
			goto HASH__MAINLOOP(keyname, val); \
		else \
			while(1) \
				if(1) { \
					goto HASH__FINISHED(keyname, val); \
				} \
				else \
					while(1) \
						if(!HT_next(obj, & (HASH__ITER(keyname, val)), &keyname, &valname)) { \
							goto HASH__FINISHED(keyname, val); \
						} \
						else \
							HASH__MAINLOOP(keyname, val) :
							
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












#endif // __sti__hash_h__
