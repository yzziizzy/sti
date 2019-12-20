#ifndef __sti__string_h__
#define __sti__string_h__



// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s);

// strdup a line
char* strlndup(const char* s);

// line count;
size_t strlinecnt(const char* s);




#endif // __sti__string_h__
