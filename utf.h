#ifndef __sti__utf_h__
#define __sti__utf_h__

/////////////////////////////////////////////////////////////////////////
//
//         NOTE:
//
//  This is a basic, general utf8/32 string handling library.
//  It is written for speed and efficiency before "correctness".
//  It will pass through any characters it does not know about.
//  It is aimed at *reasonable* support for *most* modern languages.
//  It IS NOT a full-featured, complete, correct unicode library. 
//  It DOES NOT handle every intricacy of every language, potentially
//     including your favorite language.
//  It makes no attempt, and never will, to support special features of
//     ancient, obscure, or unusual languages.
//  Character properties above FFFF are completely unsupported.
//  
/////////////////////////////////////////////////////////////////////////

// https://www.unicode.org/reports/tr44/#UnicodeData.txt


#include <string.h>

// utf8 functions

/*
Naming Convention
Mostly derived from string.h

str [n|k] r? {operation} [8|32] p?

str: consistent prefix
n: limited by bytes
k: limited by codepoints (characters)
r: reverse; starts at the end of the string
operation: see below
8: operates on utf8
32: operates on utf32
p: (utf8 only) accepts a pointer to a potentially multibyte encoded sequence instead of a 32-bit codepoint

Operations:
	cat: append onto existing string; strcat
	chr: search for a charater; strchr
	cmp: compare; strcmp
	colwsp: collapse whitespace. All sequences of whitespace are converted into a single copy of the provided character
	cpy: copy; strcpy
	cspn: return length of inverse prefix substring
	dup: duplicate into newly allocated memory; strdup
	len: calculate length; strlen 
	pbrk: search for the first of any of a set of characters
	spn: return length of prefix substring
	skip: search for the first character not in a set of characters (strspn, but returns a pointer)
	str: search for a substring; strstr
	ltrim: remove a prefix of characters in a set from the beginning of the string
	rtrim: remove a suffix of characters in a set from the end of the string
	trim: remove a sequence characters in a set from the beginning and end of the string
*/

// returns the number of characters in a utf8 string
size_t charlen8(const char* u8);

// returns a new buffer, caller must free, or NULL when the string is malformed
uint32_t* utf8_to_utf32(uint8_t* u8, size_t* outLen);


// It is the caller's responsibility to provide at least 4 bytes of output memory
// returns the number of bytes used.
int utf32_to_utf8(uint32_t u32, uint8_t* u8_out);

// returns the number of bytes needed to encode this codepoint in utf8
int utf8_bytes_needed(uint32_t u32);

// byte length of a single utf8 character, with subsequent btye format verification and null checks
int utf8_char_size(const char* u8);

// returns 1 if there are multi-byte sequences, 0 otherwise
int utf8_has_multibyte(const uint8_t* u8);


// returns NULL on not found or if codepoint is invalid
char* strchr8(const char* s, uint32_t codepoint);
// c is a pointer to a single utf8 character, up to 4 bytes
// returns NULL on not found or if codepoint is invalid
char* strchr8p(const char* s, const char* c);



char* strrchr8(const char* s, uint32_t codepoint);
char* strrchr8p(const char* s, const char* c);
// size_t strcspn8(const char* a, const char* b);
// char* strpbrk8(const char* a, const char* b);
// char* strtok8_r(char* s, const char* delim, char** saveptr);
// char* strnstr8(const char* a, const char* b, size_t len) { return strnstr(a, b, len); }


// char* strcasestr8(const char* a, const char* b);


// some normal string functions are utf8 safe. *8 versions are provided here so you don't have
//   to remember which ones are which
inline static size_t strlen8(const char* s) { return strlen(s); }

inline static char* strcat8(char* dst, const char* src) { return strcat(dst, src); }
inline static char* strncat8(char* dst, const char* src, size_t len) { return strncat(dst, src, len); }
char* strkcat8(char* dst, const char* src, size_t clen);


inline static char* strcpy8(char* dst, const char* src) { return strcpy(dst, src); }
inline static char* strncpy8(char* dst, const char* src, size_t len) { return strncpy(dst, src, len); }


inline static int   strcmp8(const char* a, const char* b) { return strcmp(a, b); }
inline static int   strncmp8(const char* a, const char* b, size_t len) { return strncmp(a, b, len); }
inline static char* strstr8(const char* a, const char* b) { return strstr(a, b); }
inline static char* strdup8(const char* const s) { return strdup(s); }
inline static char* strndup8(const char* const s, size_t len) { return strndup(s, len); }
// inline static char* strtok_r8(const char* const s, size_t len) { return strndup(s, len); }



// strtok intentionally not implemented
inline static char* strtok8(char* s, const char* delim) {
	(void)s;
	(void)delim;
	*((int*)0) = 0xBadBad; // segfault on purpose because non-reentrant fns are bad. use strtok_r. 
	return NULL;
}



// why even bother with utf16? all downsides, no upsides.



// utf32 functions


// in bytes, not including (4-byte) null terminator
size_t strlen32(const uint32_t* const s);

// in characters
size_t charlen32(const uint32_t* const s);


uint32_t* strcat32(uint32_t* dst, const uint32_t* src);
uint32_t* strncat32(uint32_t* dst, const uint32_t* src, size_t len);
uint32_t* strcpy32(uint32_t* dst, const uint32_t* src);
uint32_t* strncpy32(uint32_t* dst, const uint32_t* src, size_t len);
uint32_t* strchr32(const uint32_t* s, uint32_t c);
uint32_t* strrchr32(const uint32_t* s, uint32_t c);
uint32_t* strchrnul32(uint32_t* s, uint32_t c);
int       strcmp32(const uint32_t* a, const uint32_t* b);
int       strncmp32(const uint32_t* a, const uint32_t* b, size_t len);
size_t    strspn32(const uint32_t* s, const uint32_t* accept);
size_t    strcspn32(const uint32_t* s, const uint32_t* reject);
// uint32_t* strpbrk32(const uint32_t* a, const uint32_t* b);
// uint32_t* strstr32(const uint32_t* a, const uint32_t* b);
// uint32_t* strnstr32(const uint32_t* a, const uint32_t* b);
uint32_t* strdup32(const uint32_t* const s);
// uint32_t* strndup32(const uint32_t* const s);
// uint32_t* strtok_r32(uint32_t* s, const uint32_t* delim, uint32_t** saveptr);


// strtok intentionally not implemented
inline static uint32_t* strtok32(uint32_t* s, const uint32_t* delim) {
	(void)s;
	(void)delim;
	*((int*)0) = 0xBadBad; // segfault on purpose because non-reentrant fns are bad. use strtok_r. 
	return NULL;
}




// http://www.unicode.org/reports/tr44/#UnicodeData.txt

// TODO:
//   toupper/tolower for whole strings
//   Capitalize words
//   Character class info
//   Strip emoji
//   Convert all emoji to random list of supplied emoji
//   printf
//   trim

#endif // __sti__utf_h__
