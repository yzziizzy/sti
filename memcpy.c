
#include <stdio.h>
#include <stddef.h>
#define NO_GCC_LIBC_CALL __attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns")))


#include <immintrin.h>
#include <emmintrin.h>

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern long cacheSize;
extern long halfCacheSize;


// naive C
NO_GCC_LIBC_CALL void* memcpy_1(void* restrict dest, const void* restrict src, size_t n) {
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	while(n--) *d++ = *s++;


	return dest;
}


// unaligned avx
NO_GCC_LIBC_CALL void* memcpy_2(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	
	size_t n32s = n / 32;
	
	for(size_t j = n32s; j; j--) {
		__m256i tmp = _mm256_loadu_si256((__m256i*)s);
		_mm256_storeu_si256((__m256i*)d, tmp);
		s += 32;
		d += 32;
	}
	
	n -= n32s * 32;
	
	size_t n8s = n / 8;
	n -= n8s;
	switch(n8s) {
		case 3: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 2: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 1: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 0:
	}
	
	switch(n) {
		case  7: *d++ = *s++;
		case  6: *d++ = *s++;
		case  5: *d++ = *s++;
		case  4: *d++ = *s++;
		case  3: *d++ = *s++;
		case  2: *d++ = *s++;
		case  1: *d++ = *s++;
		case  0:
	}

	return dest;
}

// unaligned avx, alternate close
NO_GCC_LIBC_CALL void* memcpy_2b(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	
	size_t n32s = n / 32;
	
	for(size_t j = n32s; j; j--) {
		__m256i tmp = _mm256_loadu_si256((__m256i*)s);
		_mm256_storeu_si256((__m256i*)d, tmp);
		s += 32;
		d += 32;
	}
	
	n -= n32s * 32;
	
	if(n == 0) return dest;
	
	s = s - 32 + n;
	d = d - 32 + n;
	
	__m256i tmp2 = _mm256_loadu_si256((__m256i*)s);
	_mm256_storeu_si256((__m256i*)d, tmp2);

	return dest;
}


// aligned avx, requires aligned inputs
NO_GCC_LIBC_CALL void* memcpy_3(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
		
	
	size_t n32s = n / 32;
	
	for(size_t j = n32s; j; j--) {
		__m256i tmp = _mm256_load_si256((__m256i*)s);
		_mm256_store_si256((__m256i*)d, tmp);
		s += 32;
		d += 32;
	}
	
	n -= n32s * 32;
	
	size_t n8s = n / 8;
	n -= n8s * 8;
	switch(n8s) {
		case 3: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 2: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 1: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 0:
	}
	
	switch(n) {
		case  7: *d++ = *s++;
		case  6: *d++ = *s++;
		case  5: *d++ = *s++;
		case  4: *d++ = *s++;
		case  3: *d++ = *s++;
		case  2: *d++ = *s++;
		case  1: *d++ = *s++;
		case  0:
	}

	return dest;
}


// uncached aligned avx
NO_GCC_LIBC_CALL void* memcpy_4(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	
	u64 low = 31ul;
	
	size_t begin = (u64)s & low;
	
	size_t b8s = begin / 8;
	switch(b8s) {
		case 3: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 2: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 1: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 0:
	}
	
	switch(begin - b8s * 8) {
		case  7: *d++ = *s++;
		case  6: *d++ = *s++;
		case  5: *d++ = *s++;
		case  4: *d++ = *s++;
		case  3: *d++ = *s++;
		case  2: *d++ = *s++;
		case  1: *d++ = *s++;
		case  0:
	}
	
	
	n -= begin;
	
	
	size_t n32s = n / 32;
	
	for(size_t j = n32s; j; j--) {
		__m256i tmp = _mm256_stream_load_si256((__m256i*)s);
		_mm256_stream_si256((__m256i*)d, tmp);
		s += 32;
		d += 32;
	}
	
	n -= n32s * 32;
	
	size_t n8s = n / 8;
	n -= n8s * 8;
	switch(n8s) {
		case 3: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 2: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 1: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 0:
	}
	
	switch(n) {
		case  7: *d++ = *s++;
		case  6: *d++ = *s++;
		case  5: *d++ = *s++;
		case  4: *d++ = *s++;
		case  3: *d++ = *s++;
		case  2: *d++ = *s++;
		case  1: *d++ = *s++;
		case  0:
	}

	return dest;
}



// auto-aligning avx
NO_GCC_LIBC_CALL void* memcpy_5(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	
	
	u64 low = 31ul;
	
	size_t begin = (u64)s & low;
	
	size_t b8s = begin / 8;
	switch(b8s) {
		case 3: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 2: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 1: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 0:
	}
	
	switch(begin - b8s * 8) {
		case  7: *d++ = *s++;
		case  6: *d++ = *s++;
		case  5: *d++ = *s++;
		case  4: *d++ = *s++;
		case  3: *d++ = *s++;
		case  2: *d++ = *s++;
		case  1: *d++ = *s++;
		case  0:
	}
	
	
	n -= begin;
	
	size_t n32s = n / 32;
	
	for(size_t j = n32s; j; j--) {
		__m256i tmp = _mm256_load_si256((__m256i*)s);
		_mm256_storeu_si256((__m256i*)d, tmp);
		s += 32;
		d += 32;
	}
	
	n -= n32s * 32;
	
	size_t n8s = n / 8;
	n -= n8s * 8;
	switch(n8s) {
		case 3: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 2: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 1: *((u64*)d) = *((u64*)s); d += 8; s += 8;
		case 0:
	}
	
	switch(n) {
		case  7: *d++ = *s++;
		case  6: *d++ = *s++;
		case  5: *d++ = *s++;
		case  4: *d++ = *s++;
		case  3: *d++ = *s++;
		case  2: *d++ = *s++;
		case  1: *d++ = *s++;
		case  0:
	}

	return dest;
}





// auto-aligning avx, unaligned beginning
NO_GCC_LIBC_CALL void* memcpy_6_slower(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	

	switch(n) {
		#define do16 _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16;
		#define do8  *((u64*)d) = *((u64*)s); d += 8; s += 8;
		#define do4  *((u32*)d) = *((u32*)s); d += 4; s += 4;
		#define do2  *((u16*)d) = *((u16*)s); d += 2; s += 2;
		#define do1  *((u8*)d) = *((u8*)s); 
		
		case 31: do16
		case 15: do8
		case 7:  do4
		case 3:  do2
		case 1:  do1 break;
		
		case 30: do16 
		case 14: do8
		case 6:  do4
		case 2:  do2 break;
		
		case 29: do16
		case 13: do8
		case 5:  do4 do1 break;
		
		case 28: do16
		case 12: do8
		case 4:  do4 break;

		case 27: do16
		case 11: do8 do2 do1 break;
		
		case 26: do16
		case 10: do2
		case 8:  do8 break;
		
		case 25: do16
		case 9:  do8 do1 break;
		
		case 24: do16 do8 break;
		case 23: do16 do4 do2 do1 break;
		case 22: do16 do4 do2 break;
		case 21: do16 do4 do1 break;
		case 20: do16 do4 break;
		case 19: do16 do2 do1 break;
		case 18: do16 do2 break;
		case 17: do16 do1 break;
		case 16: do16 break;
		default: goto BIGGER;
	}
	return dest;


BIGGER:
	
	u64 low = 31ul;
	
	size_t begin = (u64)s & low;
	
	__m256i tmp = _mm256_loadu_si256((__m256i*)s);
	_mm256_storeu_si256((__m256i*)d, tmp);
	
	s += begin;
	d += begin;
	
	
	n -= begin;
	
	size_t n32s = n / 32;
	
	
	if(n >= halfCacheSize) { // skip the cache
		for(size_t j = n32s; j; j--) {
			__m256i tmp = _mm256_stream_load_si256((__m256i*)s);
			_mm256_storeu_si256((__m256i*)d, tmp);
			s += 32;
			d += 32;
		}
	
	}
	else { // don't skip the cache
		for(size_t j = n32s; j; j--) {
			__m256i tmp = _mm256_load_si256((__m256i*)s);
			_mm256_storeu_si256((__m256i*)d, tmp);
			s += 32;
			d += 32;
		}
	}
	
	n -= n32s * 32;
	
	if(n == 0) return dest;
	
	s = s - 32 + n;
	d = d - 32 + n;
	
	__m256i tmp2 = _mm256_loadu_si256((__m256i*)s);
	_mm256_storeu_si256((__m256i*)d, tmp2);

	return dest;
}

// auto-aligning avx, unaligned beginning
NO_GCC_LIBC_CALL void* memcpy_6(void* restrict dest, const void* restrict src, size_t n) {
	
	
	char* restrict d = (char*)dest;
	char* restrict s = (char*)src;
	
	
	if(n < 32) { // faster than   while(n--) *d++ = *s++; 
		if(n >= 16) { _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((__m128i*)s)); d += 16; s += 16; n -= 16; }
		if(n >= 8) { *((u64*)d) = *((u64*)s); d += 8; s += 8; n -= 8; }
		if(n >= 4) { *((u32*)d) = *((u32*)s); d += 4; s += 4; n -= 4; }
		if(n >= 2) { *((u16*)d) = *((u16*)s); d += 2; s += 2; n -= 2; }
		if(n >= 1) { *((u8*)d) = *((u8*)s); }
				
		return dest;
	}
	

	
	u64 low = 31ul;
	size_t n32s;
	

	int salign = (u64)s & low;
	
	// TODO: check the size where it's faster
	if(n >= halfCacheSize) { // skip the cache
		
		int dalign = (u64)d & low;
		
		if(salign == dalign) {
			size_t begin = salign;
			
			// TODO: check if it's faster to switch on the size of begin for smaller operations
			_mm256_storeu_si256((__m256i*)d, _mm256_loadu_si256((__m256i*)s));
			s += begin;
			d += begin;
			n -= begin;
			
			n32s = n / 32;
			
			for(size_t j = n32s; j; j--) {
				__m256i tmp = _mm256_stream_load_si256((__m256i*)s);
				_mm256_stream_si256((__m256i*)d, tmp);
				s += 32;
				d += 32;
			}
		}
		else {
			size_t begin = salign; // TODO: check whether it's more important to optimize for loads or stores
			// TODO: check whether it even matters at all -- UPDATE: looks like it does, and GCC doesn't handle it well
			
			// TODO: verify that these functions all actually work
			
			_mm256_storeu_si256((__m256i*)d, _mm256_loadu_si256((__m256i*)s));
			s += begin;
			d += begin;
			n -= begin;
			
			n32s = n / 32;
			
			for(size_t j = n32s; j; j--) {
				__m256i tmp = _mm256_stream_load_si256((__m256i*)s);
				_mm256_storeu_si256((__m256i*)d, tmp);
				s += 32;
				d += 32;
			}
		}
	
	}
	else { // don't skip the cache
		size_t begin = salign;
		
		_mm256_storeu_si256((__m256i*)d, _mm256_loadu_si256((__m256i*)s));
		s += begin;
		d += begin;
		n -= begin;
		
		n32s = n / 32;
		
		for(size_t j = n32s; j; j--) {
			__m256i tmp = _mm256_load_si256((__m256i*)s);
			_mm256_storeu_si256((__m256i*)d, tmp);
			s += 32;
			d += 32;
		}
	}
	
	n -= n32s * 32;
	
	if(n == 0) return dest;
	
	s = s - 32 + n;
	d = d - 32 + n;
	
	__m256i tmp2 = _mm256_loadu_si256((__m256i*)s);
	_mm256_storeu_si256((__m256i*)d, tmp2);

	return dest;
}

