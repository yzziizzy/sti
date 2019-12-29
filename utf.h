#ifndef __sti__utf_h__
#define __sti__utf_h__

/////////////////////////////////////////////////////////////////////////
//
//         NOTE:
//
//  This is a basic, general utf8/32 string handling library.
//  It is written with speed and efficiency before "correctness".
//  It will pass through any characters it does not know about.
//  It is aimed at *reasonable* support for *most* modern languages.
//  It IS NOT a full-featured, complete, correct unicode library. 
//  It DOES NOT handle every intricacy of every language, potentially
//     including your favorite language.
//  It makes no attempt, and never will, to support special features of
//     ancient, obscure, or unusual languages.
//  
/////////////////////////////////////////////////////////////////////////

// https://www.unicode.org/reports/tr44/#UnicodeData.txt


// utf8 functions


// returns the number of characters in a utf8 string
size_t charlen8(const char* u8);

// returns a new buffer, caller must free, or NULL when the string is malformed
uint32_t* utf8_to_utf32(char* u8, size_t* outLen);

// returns 1 if there are multi-byte sequences, 0 otherwise
int utf8_has_multibyte(const char* u8);


// char* strchr8(const char* s, uint32_t codepoint);
// char* strrchr8(const char* s, uint32_t codepoint);
// char* strchr8p(const char* s, char* c);
// char* strrchr8p(const char* s, char* c);
// size_t strcspn8(const char* a, const char* b);
// char* strpbrk8(const char* a, const char* b);
// char* strtok8_r(char* s, const char* delim, char** saveptr);
// char* strnstr8(const char* a, const char* b, size_t len) { return strnstr(a, b, len); }


// char* strcasestr8(const char* a, const char* b);


// some normal string functions are utf8 safe. *8 versions are provided here so you don't have
//   to remember which ones are which
inline static char* strcat8(char* dst, const char* src) { return strcat(dst, src); }
inline static char* strncat8(char* dst, const char* src, size_t len) { return strncat(dst, src, len); }
inline static char* strcpy8(char* dst, const char* src) { return strcpy(dst, src); }
inline static char* strncpy8(char* dst, const char* src, size_t len) { return strncpy(dst, src, len); }
inline static int   strcmp8(const char* a, const char* b) { return strcmp(a, b); }
inline static int   strncmp8(const char* a, const char* b, size_t len) { return strncmp(a, b, len); }
inline static char* strstr8(const char* a, const char* b) { return strstr(a, b); }
inline static char* strdup8(const char* const s) { return strdup(s); }
inline static char* strndup8(const char* const s, size_t len) { return strndup(s, len); }


// strtok intentionally not implemented
inline static char* strtok8(char* s, const char* delim) {
	(void)s;
	(void)delim;
	*((int*)0) = 1; // segfault on purpose because non-reentrant fns are bad. use strtok_r. 
	return NULL;
}



// why even bother with utf16? all downsides, no upsides.



// utf32 functions


// in bytes, not including (4-byte) null terminator
size_t strlen32(const uint32_t* const s);

// in characters
size_t charlen32(const uint32_t* const s);


// uint32_t* strcat32(uint32_t* dst, const uint32_t* src);
// uint32_t* strncat32(uint32_t* dst, const uint32_t* src, size_t len);
// uint32_t* strcpy32(uint32_t* dst, const uint32_t* src);
// uint32_t* strncpy32(uint32_t* dst, const uint32_t* src, size_t len);
// uint32_t* strchr32(const uint32_t* s, uint32_t c);
// int       strcmp32(const uint32_t* a, const uint32_t* b);
// int       strncmp32(const uint32_t* a, const uint32_t* b, size_t len);
// size_t    strcspn32(const uint32_t* a, const uint32_t* b);
// uint32_t* strpbrk32(const uint32_t* a, const uint32_t* b);
// uint32_t* strstr32(const uint32_t* a, const uint32_t* b);
// uint32_t* strnstr32(const uint32_t* a, const uint32_t* b);
uint32_t* strdup32(const uint32_t* const s);
// uint32_t* strndup32(const uint32_t* const s);
// uint32_t* strtok32_r(uint32_t* s, const uint32_t* delim, uint32_t** saveptr);


// strtok intentionally not implemented
inline static uint32_t* strtok32(uint32_t* s, const uint32_t* delim) {
	(void)s;
	(void)delim;
	*((int*)0) = 1; // segfault on purpose because non-reentrant fns are bad. use strtok_r. 
	return NULL;
}


#endif // __sti__utf_h__
