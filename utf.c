

#include <stddef.h>
#include <stdint.h>
#include <string.h>


#include "utf.h"

#define to32s(x, s) (((uint32_t)(x)) << (s))


uint32_t* utf8_to_utf32(char* u8, size_t* outLen) {
	size_t u8len = strlen(u8);
	
	uint32_t* u32 = malloc((u8len + 1) * sizeof(u32)); // just overallocate
	
	uint8_t* s = (uint8_t*)u8;
	
	int i = 0;
	while(*s) {
		if((s[0] & 0x80) == 0x00) { // single byte
			u32[i] = s[0];
			s++;
		}
		else if((s[0] & 0xe0) == 0xc0) { // two bytes
			if(s[1] == 0) goto MALFORMED;
			u32[i] = to32s(s[0] & 0x1f, 6) | to32s(s[1] & 0x3f, 0);
			s += 2;
		}
		else if((s[0] & 0xf0) == 0xe0) { // three bytes
			if(s[1] == 0 || s[2] == 0) goto MALFORMED;
			u32[i] = to32s(s[0] & 0x1f, 12) | to32s(s[1] & 0x3f, 6) | to32s(s[2] & 0x3f, 0);
			s += 3;
		}
		else if((s[0] & 0xf8) == 0xf0) { // four bytes
			if(s[1] == 0 || s[2] == 0 || s[3] == 0) goto MALFORMED;
			u32[i] = to32s(s[0] & 0x1f, 18) | to32s(s[1] & 0x3f, 12) | to32s(s[2] & 0x3f, 6) | to32s(s[3] & 0x3f, 0);
			s += 4;
		}
		
		i++;
	}
	
	u32[i] = 0;
	if(outLen) *outLen = i;
	
	return u32;
	
	
MALFORMED:
	fprintf(stderr, "Malformed UTF-8 sequence.\n");
// 	exit(1); // we ain't havin none of that shit
	free(u32);
	return NULL;
}




// returns the number of characters in a utf8 string
size_t charlen8(const char* u8) {
	size_t len = 0;
	
	uint8_t* s = (uint8_t*)u8;
	
	while(*s) {
		if((s[0] & 0x80) == 0x00) { // single byte
			s++;
		}
		else if((s[0] & 0xe0) == 0xc0) { // two bytes
			if(s[1] == 0) break; // malformed sequence
			s += 2;
		}
		else if((s[0] & 0xf0) == 0xe0) { // three bytes
			if(s[1] == 0 || s[2] == 0) break; // malformed sequence
			s += 3;
		}
		else if((s[0] & 0xf8) == 0xf0) { // four bytes
			if(s[1] == 0 || s[2] == 0 || s[3] == 0) break; // malformed sequence
			s += 4;
		}
		
		len++;
	}
	
	
	return len;
}


// returns 1 if there are multi-byte sequences, 0 otherwise
int utf8_has_multibyte(const char* u8) {
	uint8_t* s = (uint8_t*)u8;
	
	while(*s) {
		if((s[0] & 0x80) != 0x00) {
			return 1;
		}
		s++;
	}
	
	return 0;
}

// in bytes, not including (4-byte) null terminator
size_t strlen32(const uint32_t* s) {
	const uint32_t* e = s;
	while(*e) e++;
	return  (e - s);
}

// in characters
size_t charlen32(const uint32_t* s) {
	return strlen32(s) >> 2;
}


uint32_t* strcat32(uint32_t* dst, const uint32_t* src) {
	uint32_t* d = dst;
	const uint32_t* s = src;
	while(*d) d++;
	
	while(*s) *d++ = *s++;
	*d = 0;
	
	return dst;
}

uint32_t* strncat32(uint32_t* dst, const uint32_t* src, size_t len) {
	uint32_t* d = dst;
	const uint32_t* s = src;
	while(*d) d++;
	
	while(*s && len--) *d++ = *s++;
	*d = 0;
	
	return dst;
}

uint32_t* strcpy32(uint32_t* dst, const uint32_t* src) {
	uint32_t* d = dst;
	const uint32_t* s = src;
	
	while(*s) *d++ = *s++;
	*d = 0;
	
	return dst;
}

uint32_t* strncpy32(uint32_t* dst, const uint32_t* src, size_t len) {
	uint32_t* d = dst;
	const uint32_t* s = src;
	
	while(*s && len--) *d++ = *s++;
	*d = 0;
	
	return dst;
}

uint32_t* strchr32(const uint32_t* s, uint32_t c) {
	for(; *s; s++) if(*s == c) return s;
	return NULL;
}

uint32_t* strrchr32(const uint32_t* s, uint32_t c) {
	uint32_t* p = NULL;
	for(; *s; s++) if(*s == c) p = s;
	return p;
}

uint32_t* strchrnul32(uint32_t* s, uint32_t c) {
	for(; *s; s++) if(*s == c) return s;
	return s;
}

int strcmp32(const uint32_t* a, const uint32_t* b) {
	for(; *a || *b; a++, b++) {
		if(*a < *b) return -1;
		if(*a > *b) return 1;
	}
	
	return 0;
}

int strncmp32(const uint32_t* a, const uint32_t* b, size_t len) {
	for(; (*a || *b) && len--; a++, b++) {
		if(*a < *b) return -1;
		if(*a > *b) return 1;
	}
	
	return 0;
}

size_t strspn32(const uint32_t* s, const uint32_t* accept) {
	uint32_t* start, *a;;
	for(start = s; *s; ) {
		for(a = accept; *a; a++) {
			if(*a == *s) goto CONT;
		}
		goto END;
	CONT:
		s++;
	}
END:
	return s - start;
}

size_t strcspn32(const uint32_t* s, const uint32_t* accept) {
	uint32_t* start, *a;;
	for(start = s; *s; ) {
		for(a = accept; *a; a++) {
			if(*a == *s) goto END;
		}
		s++;
	}
END:
	return s - start;
}


uint32_t* strdup32(const uint32_t* const s) {
	size_t l = strlen32(s);
	uint32_t* o = malloc(l + sizeof(*s)); // +sz for the null terminator
	memcpy(o, s, (l + 1) * sizeof(*s));
	return o;
}









