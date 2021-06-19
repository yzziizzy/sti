#ifndef __sti__string_h__
#define __sti__string_h__

// Public Domain.

#include <string.h>
#include "macros.h"

// limited strspn
size_t strnspn(const char* s, size_t count, const char* accept);

// reverse strspn
size_t strrspn(const char* s, const char* accept);

// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s);

// strdup a line
char* strlndup(const char* s);

// line count;
size_t strlinecnt(const char* s);

// allocates a new buffer and calls sprintf with it
char* sprintfdup(char* fmt, ...);

// concatenate all argument strings together in a new buffer
#define strcatdup(...) strcatdup_(PP_NARG(__VA_ARGS__), __VA_ARGS__)
char* strcatdup_(size_t nargs, ...);

// concatenate all argument strings together in a new buffer,
//    with the given joining string between them
#define strjoin(j, ...) strjoin_(j, PP_NARG(__VA_ARGS__), __VA_ARGS__)
char* strjoin_(char* joiner, size_t nargs, ...);

// returns a null-terminated list of pointers to each line.
// mutates the source (replaces newlines with nulls)
char** strsplit_inplace(char* src, char delim, size_t* outLen);

// allocates individual memory chunks for each split pointer
char** strsplit(char* src, char delim, size_t* outLen);

// trim left
size_t strtriml(char* s, const char* trim);

// trim right
size_t strtrimr(char* s, const char* trim);

// both left and right
size_t strtrim(char* s, const char* trim);

// handy shortcut
static inline char* strskip(char* s, char* skip) {
	return s + strspn(s, skip);
}

// decodes strings according to the string literal rules in C
// *s is advanced to the next char
// gleefully advances the pointer through nulls like any other character
// returns 1 if the character was escaped 
// returns an error code on invalid escape sequences
int decode_c_string_char(char** s, int* c_out);



typedef struct number_parse_info {
	union {
		long double f;
		unsigned long long int n;
	};
	
	char type; // 'f', 'i'
	char base;
	
	// suffixes
	char longs; // 0 for unspecified
	char not_signed; // 0 for unspecified
} number_parse_info;

int read_c_number(char** s, number_parse_info* info);


// format in arbitrary base/charset
int sprintlongb(char* buf, int base, int64_t n, char* charset);

// returns the numerical value of a single hex digit
unsigned int decodeHexDigit(char c);

// returns rgba, with r in most significant bits and a in the least
uint32_t decodeHexColor(char* s);

// returns rgba, with r in out[0] and a in out[3], normalized to 0xFF = 1.0
void decodeHexColorNorm(char* s, float* out);


// snprintf, with a void*[] arg list
int isnprintfv(char* out, ptrdiff_t out_sz, char* fmt, void** args);


// TODO:
// collapse whitespace
// collapse ws to a single ' '
// compare functions for natural interpretations of strings (version sort)


#endif // __sti__string_h__
