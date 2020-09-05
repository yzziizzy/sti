#ifndef __sti__string_h__
#define __sti__string_h__

// Public Domain.




// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s);

// strdup a line
char* strlndup(const char* s);

// line count;
size_t strlinecnt(const char* s);

// append b to a in a new buffer
char* strappend(const char* a, const char* const b);

// returns a null-terminated list of pointers to each line.
// mutates the source (replaces newlines with nulls)
char** strsplit_inplace(char* src, char delim, size_t* outLen);


// handy shortcut
static inline char* strskip(char* s, char* skip) {
	return s + strspn(s, skip);
}


// returns the numerical calue of a single hex digit
unsigned int decodeHexDigit(char c);

// returns rgba, with r in most significant bits and a in the least
uint32_t decodeHexColor(char* s);

// returns rgba, with r in out[0] and a in out[3], normalized to 0xFF = 1.0
void decodeHexColorNorm(char* s, float* out);


// snprintf, with a void*[] arg list
int isnprintfv(char* out, ptrdiff_t out_sz, char* fmt, void** args);


// TODO:
// trim whitespace
// collapse whitespace
// collapse ws to a single ' '
// VA fn to join arbitrary number of strings
// compare functions for natural interpretations of strings (version sort)


#endif // __sti__string_h__
