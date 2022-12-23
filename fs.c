// Public Domain.


#include <stdarg.h>
#include <stdlib.h> 
#include <stddef.h> // offsetof
#include <stdio.h> // fprintf, fopen et al.
#include <string.h> // strerror
#include <ctype.h> // isspace
#include <errno.h>


#include <fnmatch.h> // fnmatch
#include <sys/types.h> // opendir
#include <dirent.h> // readdir
#include <unistd.h> // pathconf
#include <sys/stat.h>
#include <wordexp.h>

#include "fs.h"




// given a longer name so as to not conflict with other things
// handles ~ properly
// returns 0 for false, 1 for true, and 0 on any error
int is_path_a_dir(char* path) {
	int ret;
	struct stat sb;
	
	if(!path) return 0;
	
	if(path[0] == '~') {
		char* homedir, *tmp;
		
		homedir = getenv("HOME");
		tmp = path_join(homedir, path + 1);
		
		ret = stat(tmp, &sb);
		
		free(tmp);
	}
	else {
		ret = stat(path, &sb);
	}
	
	if(ret) return 0;
	
	if(sb.st_mode & S_IFDIR) return 1;
	
	return 0;
}


// returns negative on error, nonzero if scanning was halted by the callback
int recurse_dirs(
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
		fullPath = path_join(path, n);
		
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
		fullPath = path_join(path, n);
#endif
		
		if(type == DT_DIR) {
			if(flags & FSU_INCLUDE_DIRS) {
				stop = fn(fullPath, n, data);
			}
			if(depth != 0) {
				stop |= recurse_dirs(fullPath, fn, data, depth - 1, flags);
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


char* path_join_(size_t nargs, ...) {
	size_t total = 0;
	char* out, *end;
	size_t j_len;
	char* joiner = "/";
	int escape = 0;

	if(nargs == 0) return NULL;

	// calculate total buffer length
	va_list va;
	va_start(va, nargs);

	for(size_t i = 0; i < nargs; i++) {
		char* s = va_arg(va, char*);
		if(s) total += strlen(s);
	}

	va_end(va);

	j_len = strlen(joiner);
	total += j_len * (nargs - 1);

	out = malloc((total + 1) * sizeof(char*));
	end = out;

	va_start(va, nargs);

	for(size_t i = 0; i < nargs; i++) {
		char* s = va_arg(va, char*);
		size_t l = strlen(s);
		
		if(s) {
			if(l > 1) {
				escape = s[l-2] == '\\' ? 1 : 0;
			}

			if(i > 0 && (s[0] == joiner[0])) {
				s++;
				l--;
			}

			if(i > 0 && i != nargs-1 && !escape && (s[l-1] == joiner[0])) {
				l--;
			}

			if(i > 0) {
				strcpy(end, joiner);
				end += j_len;
			}

			// should be strncpy, but GCC is so fucking stupid that it
			//   has a warning about using strncpy to do exactly what 
			//   strncpy does if you read the fucking man page.
			// fortunately, we are already terminating our strings
			//   manually so memcpy is a drop-in replacement here.
			memcpy(end, s, l);
			end += l;
		}
	}

	va_end(va);

	*end = 0;

	return out;
}


// gets a pointer to the first character of the file extension, or to the null terminator if none
char* path_ext(char* path) {
	int i;
	int len = strlen(path);
	
	for(i = len - 1; i >= 0; i--) {
		char c = path[i];
		if(c == '.') return path + i + 1;
		else if(c == '/') break;
	} 
	
	return path + len;
}

// gets a pointer to the first character of the file extension, or to the null terminator if none
// also provides the length of the path without the period and extension
char* path_ext2(char* path, int* end) {
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
char* read_whole_file(char* path, size_t* srcLen) {
	return readWholeFileExtra(path, 0, srcLen);
}

// reserves extra space in memory just in case you want to append a \n or something
// srcLen reflects the length of the content, not the allocation
char* read_whole_file_extra(char* path, size_t extraAlloc, size_t* srcLen) {
	size_t fsize, total_read = 0, bytes_read;
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
	
	while(total_read < fsize) {
		bytes_read = fread(contents + total_read, sizeof(char), fsize - total_read, f);
		total_read += bytes_read;
	}
	
	contents[fsize] = 0;
	
	fclose(f);
	
	if(srcLen) *srcLen = fsize + 1;
	
	return contents;
}


// works like realpath(), except also handles ~/
char* resolve_path(char* in) {
	int tmp_was_malloced = 0;
	char* out, *tmp;
	
	if(!in) return NULL;
	
	// skip leading whitespace
	while(isspace(*in)) in++;
	
	// handle home dir shorthand
	if(in[0] == '~') {
		char* home = getenv("HOME");
		
		tmp_was_malloced = 1;
		tmp = malloc(sizeof(*tmp) * (strlen(home) + strlen(in) + 2));
		
		strcpy(tmp, home);
		strcat(tmp, "/"); // just in case
		strcat(tmp, in + 1);
	}
	else tmp = in;
	
	out = realpath(tmp, NULL);
	
	if(tmp_was_malloced) free(tmp);
	
	return out;
}


// works like wordexp, except accepts a list of ;-separated paths
//   and returns an array of char*'s, all allocated with normal malloc
char** multi_wordexp_dup(char* input, size_t* out_len) {
	char** out; 
	wordexp_t p;
	
	char* in = input;
	char* head = input;
	
	size_t alloc = 128;
	char* buf = malloc(alloc * sizeof(*buf));
	
	int flags = WRDE_NOCMD;
	
	while(1) {
		if(*in == ';' || *in == 0) {
			
			size_t len = in - head;
			if(len) {
				if(len + 1 > alloc) {
					alloc *= 2;
					if(alloc < len + 1) alloc = len + 1;
					
					buf = realloc(buf, alloc * sizeof(*buf));
				} 
				
				strncpy(buf, head, len);
				buf[len] = 0;
				
				wordexp(buf, &p, flags);
				flags |= WRDE_APPEND;
			}
			
			head = in + 1;
		}
		else if(*in == '\\') {
			in++;
		}
		
		if(!*in) break;
			
		in++;	
	};
	
	// fill the output array
	out = malloc((p.we_wordc + 1) * sizeof(*out));
	out[p.we_wordc] = NULL;
	
	for(unsigned int i = 0; i < p.we_wordc; i++) {
		out[i] = strdup(p.we_wordv[i]);	
	}
	
	if(out_len) *out_len = p.we_wordc;
	
	
	free(buf);
	wordfree(&p);
	
	return out;
}





static int rglob_fn(char* full_path, char* file_name, void* _results) {
	rglob* res = (rglob*)_results;
	
	if(0 == fnmatch(res->pattern, file_name, 0)) {
		if(res->len >= res->alloc) {
			res->alloc *= 2;
			res->entries = realloc(res->entries, sizeof(*res->entries) * res->alloc);
		}
		
		res->entries[res->len].type = -1;
		res->entries[res->len].full_path = strdup(full_path);
		res->entries[res->len].file_name = strdup(file_name);
		res->len++;
	}
	
	return 0;
}

void recursive_glob(char* base_path, char* pattern, int flags, rglob* results) {
	
	// to pass into recurse_dirs()
	results->pattern = pattern;
	results->len = 0;
	results->alloc = 32;
	results->entries = malloc(sizeof(*results->entries) * results->alloc);
	
	recurse_dirs(base_path, rglob_fn, results, -1, flags);
}




