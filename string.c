// Public Domain.


#include <string.h>

#include "string.h"





// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s) {
	char* n;
	
	n = strchr(s, '\n');
	if(!n) return strlen(s);
	
	return n - s;
}

// strdup a line
char* strlndup(const char* s) {
	return strndup(s, strlnlen(s));
}

// line count;
size_t strlinecnt(const char* s) {
	size_t n;

	if(!*s) return 0;
	
	n = 1;
	while(*s) // just to make you cringe
		if(*s++ == '\n') 
			n++;
	
	return n;
}



// append b to a in a new buffer
char* strappend(const char* a, const char* const b) {
	if(a == NULL) return strdup(b);
	if(b == NULL) return strdup(a);
	
	size_t la = strlen(a);
	size_t lb = strlen(b);
	char* o = malloc(la + lb + 1);
	strcpy(o, a);
	strcpy(o + la, b);
	o[la + lb] = '\0';
	return o;
}



// returns a null-terminated list of pointers to each line.
// mutates the source (replaces newlines with nulls)
char** strsplit_inplace(char* src, char delim, size_t* outLen) {
	size_t alloc = 8;
	size_t len = 0;
	char** out = malloc(alloc * sizeof(*out));
	
	char* start = src;
	
	
	for(size_t i = 0; src[i] != 0; i++) {
		if(src[i] == delim) {
			src[i] = 0; // put in a null terminator
			
			
			out[len++] = start;
			start = src + i + 1;
			
			// always have two extra for the end
			if(len + 1 >= alloc){
				alloc *= 2;
				out = realloc(out, alloc * sizeof(*out));
			}
			
		}
	}
	
// 	out[len++] = start;
	out[len++] = NULL;
	
	if(outLen) *outLen = len;
	
	return out;
}





unsigned int decodeHexDigit(char c) {
	if(c >= '0' && c <= '9') {
		return c - '0';
	}
	else if(c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}
	else if(c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}
	return 0;
}
/*
static double nibbleHexNorm(char* s) {
	if(s[0] == '\0' || s[1] == '\0') return 0.0;
	double d = (decodeHexDigit(s[0]) * 16.0) + decodeHexDigit(s[1]);
	return d / 256.0;
}
*/

// returns rgba, with r in most significant bits and a in the least
uint32_t decodeHexColor(char* s) {
	int i;
	unsigned short c[4] = {0,0,0,255};
	
	// throw away any leading BS
	char* e = s + strlen(s);
	if(s[0] == '#') s++;
	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
	
	// the actual decoding
	for(i = 0; i < 4 && s < e; i++) {
		c[i] = (decodeHexDigit(s[0]) << 4) + decodeHexDigit(s[1]);
		c[i] = c[i] > 255 ? 255 : c[i];
		s += 2;
	}
	
// 	printf(" color: %d,%d,%d,%d\n", c[0], c[1], c[2], c[3]);
	
	uint32_t o = 
		(((uint32_t)c[0]) << 24) |
		(((uint32_t)c[1]) << 16) |
		(((uint32_t)c[2]) << 8) |
		(((uint32_t)c[3]) << 0);
	
	return o;
}


void decodeHexColorNorm(char* s, float* out) {
	int i;
	out[0] = 0.0;
	out[1] = 0.0;
	out[2] = 0.0;
	out[3] = 1.0;
	
	// throw away any leading BS
	char* e = s + strlen(s);
	if(s[0] == '#') s++;
	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
	
	// the actual decoding
	for(i = 0; i < 4 && s < e; i++) {
		int n = (decodeHexDigit(s[0]) << 4) + decodeHexDigit(s[1]);
		out[i] = n / 255.0; 
		s += 2;
	}
}

