#ifndef __sti__fs_h__
#define __sti__fs_h__

// Public Domain.



#include <stddef.h> // size_t
#include "macros.h"

typedef struct rglob_entry {
	char type;
	char* full_path;
	char* file_name;
//	char* dir_name;
} rglob_entry;

typedef struct rglob {
	char* pattern;

	int len;
	int alloc;
	rglob_entry* entries;
	
} rglob;


void recursive_glob(char* base_path, char* pattern, int flags, rglob* results);


// given a longer name so as to not conflict with other things
// handles ~ properly
// returns 0 for false, 1 for true, and 0 on any error
int is_path_a_dir(char* path);

// join all path segments in a new buffer
#define path_join(...) path_join_(PP_NARG(__VA_ARGS__), __VA_ARGS__)
char* path_join_(size_t nargs, ...);

// gets a pointer to the first character of the file extension, or to the null terminator if none
char* path_ext(char* path);

// gets a pointer to the first character of the file extension, or to the null terminator if none
// also provides the length of the path without the period and extension
char* path_ext2(char* path, int* end);


// returns a null terminated string. srcLen does NOT include the null terminator
// nulls inside the string are not escaped or removed; the first null is not
//   necessarily the terminating null
char* read_whole_file(char* path, size_t* srcLen);

// reserves extra space in memory just in case you want to append a \n or something
// srcLen reflects the length of the content, not the allocation
char* read_whole_file_extra(char* path, size_t extraAlloc, size_t* srcLen);


char* write_whole_file(char* path, void* data, size_t len);

// return 0 to continue, nonzero to stop all directory scanning
typedef int (*readDirCallbackFn)(char* /*fullPath*/, char* /*fileName*/, void* /*data*/);

#define FSU_EXCLUDE_HIDDEN     (1<<0)
#define FSU_NO_FOLLOW_SYMLINKS (1<<1)
#define FSU_INCLUDE_DIRS       (1<<2)
#define FSU_EXCLUDE_FILES      (1<<3)

// returns negative on error, nonzero if scanning was halted by the callback
int recurse_dirs(
	char* path, 
	readDirCallbackFn fn, 
	void* data, 
	int depth, 
	unsigned int flags
);


// works like realpath(), except also handles ~/
char* resolve_path(char* in);

// works like wordexp, except accepts a list of ;-separated paths
//   and returns an array of char*'s, all allocated with normal malloc
char** multi_wordexp_dup(char* input, size_t* out_len);


#ifndef NO_STI_V0_COMPAT
	#define recurseDirs(...) recurse_dirs(__VA_ARGS__)
	#define readWholeFileExtra(...) read_whole_file_extra(__VA_ARGS__)
	#define readWholeFile(...) read_whole_file(__VA_ARGS__)
	#define pathExt(...) path_ext(__VA_ARGS__)
	#define pathExt2(...) path_ext2(__VA_ARGS__)
#endif


#endif // __sti__fs_h__
