#ifndef __sti__string_h__
#define __sti__string_h__

// Public Domain.


int decodeHexDigit(char c);


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


// TODO:
// trim whitespace
// collapse whitespace
// collapse ws to a single ' '
// VA fn to join arbitrary number of strings
// compare functions for natural interpretations of strings (version sort)


#endif // __sti__string_h__
