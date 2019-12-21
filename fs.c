// Public Domain.


#include <stdlib.h> 
#include <stddef.h> // offsetof
#include <stdio.h> // fprintf, fopen et al.
#include <string.h> // strerror
#include <errno.h>


#include <sys/types.h> // opendir
#include <dirent.h> // readdir
#include <unistd.h> // pathconf
#include <sys/stat.h>

#include "fs.h"



// returns negative on error, nonzero if scanning was halted by the callback
int recurseDirs(
	char* path, 
	readDirCallbackFn fn, 
	void* data, 
	int depth, 
	unsigned int flags
) {
	
	DIR* derp;
	struct dirent* result;
	int stop = 0;
	
	if(fn == NULL) {
		fprintf(stderr, "Error: readAllDir called with null function pointer.\n");
		return -1;
	}
	
	derp = opendir(path);
	if(derp == NULL) {
		fprintf(stderr, "Error opening directory '%s': %s\n", path, strerror(errno));
		return -1;
	}
	
	
	while((result = readdir(derp)) && !stop) {
		char* n = result->d_name;
		unsigned char type = DT_UNKNOWN;
		char* fullPath;
		
		// skip self and parent dir entries
		if(n[0] == '.') {
			if(n[1] == '.' && n[2] == 0) continue;
			if(n[1] == 0) continue;
			
			if(flags & FSU_EXCLUDE_HIDDEN) continue;
		}

#ifdef _DIRENT_HAVE_D_TYPE
		type = result->d_type; // the way life should be
#else
		// do some slow extra bullshit to get the type
		fullPath = pathJoin(path, n);
		
		struct stat upgrade_your_fs;
		
		lstat(fullPath, &upgrade_your_fs);
		
		if(S_ISREG(upgrade_your_fs.st_mode)) type = DT_REG;
		else if(S_ISDIR(upgrade_your_fs.st_mode)) type = DT_DIR;
		else if(S_ISLNK(upgrade_your_fs.st_mode)) type = DT_LNK;
#endif
		
		if(flags & FSU_NO_FOLLOW_SYMLINKS && type == DT_LNK) {
			continue;
		}
		
#ifdef _DIRENT_HAVE_D_TYPE
		fullPath = pathJoin(path, n);
#endif
		
		if(type == DT_DIR) {
			if(flags & FSU_INCLUDE_DIRS) {
				stop = fn(fullPath, n, data);
			}
			if(depth != 0) {
				stop |= recurseDirs(fullPath, fn, data, depth - 1, flags);
			}
		}
		else if(type == DT_REG) {
			if(!(flags & FSU_EXCLUDE_FILES)) {
				stop = fn(fullPath, n, data);
			}
		}
		
		free(fullPath);
	}
	
	
	closedir(derp);
	
	return stop;
}



char* pathJoin(char* a, char* b) {
	int alen, blen, extra = 0;
	char* o;
	
	
	alen = a ? strlen(a) : 0;
	blen = b ? strlen(b) : 0;
	
	o = malloc(alen + blen + 2);
	
	strcpy(o, a ? a : "");
	if(alen > 0 && a[alen - 1] != '/') {
		o[alen] = '/';
		extra = 1;
	}
	strcpy(o + alen + extra, b ? b : "");
	o[alen + blen + extra] = 0; 
	
	return o;
}


// gets a pointer to the first character of the file extension, or to the null terminator if none
char* pathExt(char* path) {
	int i;
	int len = strlen(path);
	
	for(i = len - 1; i >= 0; i--) {
		char c = path[i];
		if(c == '.') return path + i;
		else if(c == '/') break;
	} 
	
	return path + len;
}

// gets a pointer to the first character of the file extension, or to the null terminator if none
// also provides the length of the path without the period and extension
char* pathExt2(char* path, int* end) {
	int i;
	int len = strlen(path);
	
	for(i = len - 1; i >= 0; i--) {
		char c = path[i];
		if(c == '.') {
			if(end) *end = i > 0 ? i : 0; 
			return path + i + 1;
		}
		else if(c == '/') break;
	} 
	
	if(end) *end = len;
	return path + len;
}








// returns a null terminated string. srcLen does NOT include the null terminator
// nulls inside the string are not escaped or removed; the first null is not
//   necessarily the terminating null
char* readWholeFile(char* path, size_t* srcLen) {
	return readWholeFileExtra(path, 0, srcLen);
}

// reserves extra space in memory just in case you want to append a \n or something
// srcLen reflects the length of the content, not the allocation
char* readWholeFileExtra(char* path, size_t extraAlloc, size_t* srcLen) {
	size_t fsize;
	char* contents;
	FILE* f;
	
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "Could not open file \"%s\"\n", path);
		return NULL;
	}
	
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	rewind(f);
	
	contents = malloc(fsize + extraAlloc + 1);
	
	fread(contents, sizeof(char), fsize, f);
	contents[fsize] = 0;
	
	fclose(f);
	
	if(srcLen) *srcLen = fsize + 1;
	
	return contents;
}
