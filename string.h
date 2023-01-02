#ifndef __sti__string_h__
#define __sti__string_h__

// Public Domain.


#ifndef STI_REPLACE_STD_STRING
	#include <string.h>
#endif

#include "macros.h"

// limited strspn
size_t strnspn(const char* s, size_t count, const char* accept);

// reverse strspn
size_t strrspn(const char* s, const char* accept);

// reverse strstr
const char* strrstr(const char* haystack, const char* needle);

// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s);

// strdup a line
char* strlndup(const char* s);

// use alloca
//char* strdupa(const char* s);

// TODO: string-reverse

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





#ifdef STI_REPLACE_STD_STRING
	
	

//	void* memccpy(void *restrict, const void *restrict, int, size_t);
#define memccpy __builtin_memccpy

//	void* memchr(const void *, int, size_t);
#define memchr __builtin_memchr

//	int   memcmp(const void *, const void *, size_t);
#define memcmp __builtin_memcmp

#if !__has_builtin(__builtin_memcpy)
	void* memcpy(void* restrict dest, const void *restrict src, size_t n);
#else
	#define memcpy __builtin_memcpy
#endif

//	void* memmove(void*, const void *, size_t);
#define memmove __builtin_memmove

#if !__has_builtin(__builtin_memset)
	void* memset(void* s, int c, size_t n);
#else
	#define memset __builtin_memset
#endif

//	char* stpcpy(char *restrict, const char *restrict);
#define stpcpy __builtin_stpcpy

//	char* stpncpy(char *restrict, const char *restrict, size_t);
#define stpncpy __builtin_stpncpy

//	char* strcat(char *restrict, const char *restrict);
#define strcat __builtin_strcat

//	char* strchr(const char *, int);
#define strchr __builtin_strchr

//	int   strcmp(const char *, const char *);
#define strcmp __builtin_strcmp

//	int   strcoll(const char *, const char *);
#define strcoll __builtin_strcoll

//	int   strcoll_l(const char *, const char *, locale_t);
#define strcoll_l __builtin_strcoll_l

//	char* strcpy(char *restrict, const char *restrict);
#define strcpy __builtin_strcpy

//	size_t strcspn(const char *, const char *);
#define strcspn __builtin_strcspn

//	char* strdup(const char *);
#define strdup __builtin_strdup

//	char* strerror(int);
#define strerror __builtin_strerror

//	char* strerror_l(int, locale_t);
#define strerror_l __builtin_strerror_l

//	int strerror_r(int, char *, size_t);
#define strerror_r __builtin_strerror_r

//	size_t strlen(const char *);
#define strlen __builtin_strlen

//	char* strncat(char *restrict, const char *restrict, size_t);
#define strncat __builtin_strncat

//	int strncmp(const char *, const char *, size_t);
#define strncmp __builtin_strncmp

//	char* strncpy(char *restrict, const char *restrict, size_t);
#define strncpy __builtin_strncpy

//	char* strndup(const char *, size_t);
#define strndup __builtin_strndup

//	size_t strnlen(const char *, size_t);
#define strnlen __builtin_strnlen

//	char* strpbrk(const char *, const char *);
#define strpbrk __builtin_strpbrk

//	char* strrchr(const char *, int);
#define strrchr __builtin_strrchr

//	char* strsignal(int);
#define strsignal __builtin_strsignal

//	size_t strspn(const char *, const char *);
#define strspn __builtin_strspn

//	char* strstr(const char *, const char *);
#define strstr __builtin_strstr

//	char* strtok(char *restrict, const char *restrict);
#define strtok __builtin_strtok

//	char* strtok_r(char *restrict, const char *restrict, char **restrict);
#define strtok_r __builtin_strtok_r

//	size_t strxfrm(char *restrict, const char *restrict, size_t);
#define strxfrm __builtin_strxfrm

//	size_t strxfrm_l(char *restrict, const char *restrict, size_t, locale_t);
#define strxfrm_l __builtin_strxfrm_l

	
#endif




#endif // __sti__string_h__
