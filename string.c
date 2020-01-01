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
			
			// always have an extra 
			if(len >= alloc){
				alloc *= 2;
				out = realloc(out, alloc * sizeof(*out));
			}
			
		}
	}
	
	out[len++] = start;
	
	if(outLen) *outLen = len;
	
	return out;
}





int decodeHexDigit(char c) {
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

Vector4 sexp_argAsColor(sexp* x, int argn) {
	int i;
	union {
		Vector4 c;
		float f[4];
	} u;
	
	u.c.x = 0.0;
	u.c.y = 0.0;
	u.c.z = 0.0;
	u.c.w = 1.0; // default alpha is 1.0 

	if(VEC_LEN(&x->args) < argn) return u.c;
	sexp* arg = VEC_ITEM(&x->args, argn);
	
	if(arg->type == 0) { // it's an s-expression
		for(i = 0; i < VEC_LEN(&arg->args); i++) { 
			u.f[i] = sexp_argAsDouble(arg, i);
		}
	}
	else { // it's a literal
		// throw away any leading BS
		char* s = arg->str;
		char* e = arg->str + strlen(arg->str);
		if(s[0] == '#') s++;
		if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
		
		for(i = 0; i < 4 && s < e; i++) {
			u.f[i] = nibbleHexNorm(s);
			s += 2;
		}
	}
	
	printf("color: %f,%f,%f,%f\n", u.c.x, u.c.y, u.c.z, u.c.w);
	
	return u.c;
}
*/


