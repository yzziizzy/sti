
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>

#include "proc.h"





char* systemfdup(size_t* out_len, char const * const fmt, ...) {
	
	va_list va;
	
	va_start(va, fmt);
	size_t n = vsnprintf(NULL, 0, fmt, va);
	char* cmd = malloc(n + 1);
	va_end(va);
	
	va_start(va, fmt);
	vsnprintf(cmd, n + 1, fmt, va);
	va_end(va);
	
	

	FILE* f = popen(cmd, "r");
	free(cmd);
	if(!f) {
		return NULL;
	}
	
	size_t totalLen = 0;
	
	// collect a sequence of small buffers
	size_t buf_alloc = 4096;
	size_t buflist_alloc = 16;
	size_t buflist_len = 1;
	void** buflist = malloc(buflist_alloc * sizeof(*buflist));
	buflist[0] = malloc(buf_alloc);
	
	int fill = 0;
	while(!feof(f)) {
		ssize_t avail = buf_alloc - fill;
		if(avail <= 0) {
			if(buflist_len >= buflist_alloc) {
				buflist_alloc *= 2;
				buflist = realloc(buflist, buflist_alloc * sizeof(*buflist));
			}
		
			buflist[buflist_len++] = malloc(buf_alloc);
		}
	
		int r = fread(buflist[buflist_len - 1] + fill, 1, avail, f); 
		totalLen += r;
		fill += r;
	}
	
	pclose(f);
	
	// copy everything to one continuous buffer
	char* out = malloc(totalLen + 1); // we add a null terminator just to be nice
	char* cursor = out;
	for(int i = 0; i < buflist_len; i++) {
		memcpy(cursor, buflist[i], i == buflist_len - 1 ? fill : buf_alloc);
		free(buflist[i]);
	}
	free(buflist);
	
	out[totalLen] = 0;
	
	if(out_len) *out_len = totalLen;
	return out;
}


