

#include <stddef.h>
#include <stdint.h>
#include <string.h>


#include "utf.h"

#define to32s(x, s) (((uint32_t)(x)) << (s))


uint32_t* utf8_to_utf32(uint8_t* u8, size_t* outLen) {
	size_t u8len = strlen((char*)u8);
	
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


// It is the caller's responsibility to provide at least 4 bytes of output memory
// returns the number of bytes used.
int utf32_to_utf8(uint32_t u32, uint8_t* u8_out) {
	
	if(u32 < 0x80) { // one byte
		u8_out[0] = u32;
		return 1;
	}
	else if(u32 < 0x800) { // two bytes
		u8_out[0] = ((u32 >> 6) & 0x1f) | 0xc0;
		u8_out[1] = (u32 & 0x3f) | 0x80;
		return 2;
	}
	else if(u32 < 0x10000) { // three bytes
		u8_out[0] = ((u32 >> 12) & 0x0f) | 0xe0;
		u8_out[1] = ((u32 >> 6) & 0x3f) | 0x80;
		u8_out[2] = (u32 & 0x3f) | 0x80;
		return 3;
	}
	else if(u32 <= 0x7ffffff) { // four bytes. we gleefully encode up to the physical limit because they'll probably expand to it in the future.
		u8_out[0] = ((u32 >> 18) & 0x07) | 0xf0;
		u8_out[1] = ((u32 >> 12) & 0x3f) | 0x80;
		u8_out[2] = ((u32 >> 6) & 0x3f) | 0x80;
		u8_out[3] = (u32 & 0x3f) | 0x80;
		return 4;
	}
	
	return 5;
}


int utf8_bytes_needed(uint32_t u32) {
	if(u32 < 0x80) { // one byte
		return 1;
	}
	else if(u32 < 0x800) { // two bytes
		return 2;
	}
	else if(u32 < 0x10000) { // three bytes
		return 3;
	}
	else if(u32 <= 0x7ffffff) { // four bytes. we gleefully encode up to the physical limit because they'll probably expand to it in the future.
		return 4;
	}
	
	return 5;
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
int utf8_has_multibyte(const uint8_t* u8) {
	uint8_t* s = (uint8_t*)u8;
	
	while(*s) {
		if((s[0] & 0x80) != 0x00) {
			return 1;
		}
		s++;
	}
	
	return 0;
}


// byte length of a single utf8 character, with subsequent btye format verification and null checks
int utf8_char_size(const char* u8) {
	if((u8[0] & 0x80) == 0x00) { // single byte
		return 1;
	}
	else if((u8[0] & 0xe0) == 0xc0) { // two bytes
		if((u8[1] == 0) || ((u8[1] & 0xc0) != 0x80)) goto MALFORMED_1;
		return 2;
	}
	else if((u8[0] & 0xf0) == 0xe0) { // three bytes
		if((u8[1] == 0) || ((u8[1] & 0xc0) != 0x80)) goto MALFORMED_1;
		if((u8[2] == 0) || ((u8[2] & 0xc0) != 0x80)) goto MALFORMED_2;
		return 3;
	}
	else if((u8[0] & 0xf8) == 0xf0) { // four bytes
		if((u8[1] == 0) || ((u8[1] & 0xc0) != 0x80)) goto MALFORMED_1;
		if((u8[2] == 0) || ((u8[2] & 0xc0) != 0x80)) goto MALFORMED_2;
		if((u8[3] == 0) || ((u8[3] & 0xc0) != 0x80)) goto MALFORMED_3;
		return 4;
	}
	
	
	// the character lies outside of known utf8 encodings. 
	// just eat one byte at a times and hope for recovery
	return 1;
	
MALFORMED_3:
	return 3;
MALFORMED_2:
	return 2;
MALFORMED_1:
	return 1;
}


char* strkcat8(char* dst, const char* src, size_t clen) {
	size_t b = 0; // bytes
	
	uint8_t* ud = (uint8_t*)dst;
	uint8_t* us = (uint8_t*)src;
	
	while(*ud) ud++; // skip to the end of dst
	
	for(size_t c = 0; c < clen; c++) {
		int sz = utf8_char_size((char*)us + b);
		
		for(int i = 0; i < sz; i++) {
			if(!us[b]) goto NULL_TERM;
			
			ud[b] = us[b];
			b++;
		}
	}

NULL_TERM:

	ud[b] = 0;
	
	return dst;
}


// returns NULL on not found or if codepoint is invalid
char* strchr8(const char* s, uint32_t c32) {
	uint8_t c8[5];
	int sz;
	
	sz = utf32_to_utf8(c32, c8);
	
	switch(sz) {
		case 1: return strchr(s, c32);
		case 2:
		case 3:
		case 4:
			c8[sz] = 0;
			return strstr(s, (char*)c8);
		
		default:
			return NULL;
	}
}

char* strrchr8(const char* s, uint32_t c32) {
	uint8_t c8[5];
	int sz;
	uint8_t* us = (uint8_t*)s;
	
	sz = utf32_to_utf8(c32, c8);
	
	const uint8_t* p = NULL;
	
	for(; *us; us++) {
		if(*us == c8[0]) {
			
			// check for a full match of the entire sequence
			for(int n = 1;; n++) {
				if(n >= sz) {
					// success. save the starting spot
					p = us;
					break;
				}
				
				if(us[n] != c8[n]) break; // match failed
			}
		}
	}
	
	return (char*)p;
}

// c is a pointer to a single utf8 character, up to 4 bytes
// returns NULL on not found or if codepoint is invalid
char* strchr8p(const char* s, const char* c) {
	uint8_t c8[5];
	int sz;
	
	sz = utf8_char_size(c);
	for(int i = 0; i < sz; i++) c8[i] = c[i];
	c8[sz] = 0;
	
	return strstr(s, (char*)c8);
}

// c is a pointer to a single utf8 character, up to 4 bytes
// returns NULL on not found or if codepoint is invalid
char* strrchr8p(const char* s, const char* c) {
	int sz;
	uint8_t* us = (uint8_t*)s;
	
	sz = utf8_char_size(c);
	
	const uint8_t* p = NULL;
	
	for(; *us; us++) {
		if(*us == c[0]) {
			
			// check for a full match of the entire sequence
			for(int n = 1;; n++) {
				if(n >= sz) {
					// success. save the starting spot
					p = us;
					break;
				}
				
				if(us[n] != c[n]) break; // match failed
			}
		}
	}
	
	return (char*)p;
}



char* strnchr8(const char* src, uint32_t c32, size_t blen) {
	uint8_t c8[5];
	int sz;
	uint8_t* us = (uint8_t*)src;
	
	sz = utf32_to_utf8(c32, c8);
	
	for(int b = 0; b < blen; b++) {
		if(b >= blen) return NULL;
		for(int x = 0; x < sz; x++) {
			if(c8[x] != us[b + x] || b + x >= blen) {
				// failed match
				goto RETRY;
			}
			 
		}
			
		return (char*)(us + b);
			
	RETRY:
	}
	
	return NULL;
}


char* strkcpy8(char* dst, const char* src, size_t clen) {
	size_t b = 0; // bytes
	
	uint8_t* ud = (uint8_t*)dst;
	uint8_t* us = (uint8_t*)src;
	
	for(size_t c = 0; c < clen; c++) {
		int sz = utf8_char_size((char*)us + b);
		
		for(int i = 0; i < sz; i++) {
			if(!us[b]) goto NULL_TERM;
			
			ud[b] = us[b];
			b++;
		}
	}

NULL_TERM:
	ud[b] = 0;
	
	return dst;
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
	for(; *s; s++) if(*s == c) return (uint32_t*)s;
	return NULL;
}

uint32_t* strrchr32(const uint32_t* s, uint32_t c) {
	const uint32_t* p = NULL;
	for(; *s; s++) if(*s == c) p = s;
	return (uint32_t*)p;
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
	const uint32_t* start, *a;;
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
	const uint32_t* start, *a;;
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









