#ifndef __sti__fs_h__
#define __sti__fs_h__

#include <stddef.h> // size_t



char* pathJoin(char* a, char* b); 

// gets a pointer to the first character of the file extension, or to the null terminator if none
char* pathExt(char* path);

// gets a pointer to the first character of the file extension, or to the null terminator if none
// also provides the length of the path without the period and extension
char* pathExt2(char* path, int* end);



// returns a null terminated string. srcLen does NOT include the null terminator
// nulls inside the string are not escaped or removed; the first null is not
//   necessarily the terminating null
char* readWholeFile(char* path, size_t* srcLen);

// reserves extra space in memory just in case you want to append a \n or something
// srcLen reflects the length of the content, not the allocation
char* readWholeFileExtra(char* path, size_t extraAlloc, size_t* srcLen);


// return 0 to continue, nonzero to stop all directory scanning
typedef int (*readDirCallbackFn)(char* /*fullPath*/, char* /*fileName*/, void* /*data*/);

#define FSU_EXCLUDE_HIDDEN     (1<<0)
#define FSU_NO_FOLLOW_SYMLINKS (1<<1)
#define FSU_INCLUDE_DIRS       (1<<2)
#define FSU_EXCLUDE_FILES      (1<<3)

// returns negative on error, nonzero if scanning was halted by the callback
int recurseDirs(
	char* path, 
	readDirCallbackFn fn, 
	void* data, 
	int depth, 
	unsigned int flags
);



#endif // __sti__fs_h__
