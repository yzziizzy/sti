





typedef struct strlist {
	int len;
	int alloc;
	char** entries;
} strlist;


#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...) N
#define PP_RSEQ_N() 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_NARG(...)  PP_NARG_(__VA_ARGS__, PP_RSEQ_N())



void strlist_init(strlist* sl) {
	sl->len = 0;
	sl->alloc = 32;
	sl->entries = malloc(sl->alloc * sizeof(*sl->entries));
}
strlist* strlist_new() {
	strlist* sl = malloc(sizeof(*sl));
	strlist_init(sl);
	return sl;
}
void strlist_push(strlist* sl, char* e) {
	check_alloc(sl);
	sl->entries[sl->len++] = e;
}


typedef unsigned long hash_t;

hash_t strhash(char* str) {
	unsigned long h = 0;
	int c;

	while(c = *str++) {
		h = c + (h << 6) + (h << 16) - h;
	}
	return h;
}
hash_t strnhash(char* str, size_t n) {
	unsigned long h = 0;
	int c;

	while((c = *str++) && n--) {
		h = c + (h << 6) + (h << 16) - h;
	}
		
	return h;
}



size_t list_len(char** list) {
	size_t total = 0;
	for(; *list; list++) total++;
	return total;
}










char** concat_lists_(int nargs, ...) {
	size_t total = 0;
	char** out, **end;

	if(nargs == 0) return NULL;

	// calculate total list length
	va_list va;
	va_start(va, nargs);

	for(size_t i = 0; i < nargs; i++) {
		char** s = va_arg(va, char**);
		if(s) total += list_len(s);
	}

	va_end(va);

	out = malloc((total + 1) * sizeof(char**));
	end = out;

	va_start(va, nargs);
	
	// concat lists
	for(size_t i = 0; i < nargs; i++) {
		char** s = va_arg(va, char**);
		size_t l = list_len(s);
		
		if(s) {
			memcpy(end, s, l * sizeof(*s));
			end += l;
		}
	}

	va_end(va);

	*end = 0;

	return out;
}


char* join_str_list(char* list[], char* joiner) {
	size_t list_len = 0;
	size_t total = 0;
	size_t jlen = strlen(joiner);
	
	// calculate total length
	for(int i = 0; list[i]; i++) {
		list_len++;
		total += strlen(list[i]);
	}
	
	if(total == 0) return strdup("");
	
	total += (list_len - 1) * jlen;
	char* out = malloc((total + 1) * sizeof(*out));
	
	char* end = out;
	for(int i = 0; list[i]; i++) {
		char* s = list[i];
		size_t l = strlen(s);
		
		if(i > 0) {
			memcpy(end, joiner, jlen);
			end += jlen;
		}
		
		if(s) {
			memcpy(end, s, l);
			end += l;
		}
		
		total += strlen(list[i]);
	}
	
	*end = 0;
	
	return out;
}

// concatenate all argument strings together in a new buffer
char* strcatdup_(size_t nargs, ...) {
	size_t total = 0;
	char* out, *end;
	
	if(nargs == 0) return NULL;
	
	// calculate total buffer len
	va_list va;
	va_start(va, nargs);
	
	for(size_t i = 0; i < nargs; i++) {
		char* s = va_arg(va, char*);
		if(s) total += strlen(s);
	}
	
	va_end(va);
	
	out = malloc((total + 1) * sizeof(char*));
	end = out;
	
	va_start(va, nargs);
	
	for(size_t i = 0; i < nargs; i++) {
		char* s = va_arg(va, char*);
		if(s) {
			strcpy(end, s); // not exactly the ost efficient, but maybe faster than
			end += strlen(s); // a C version. TODO: test the speed
		};
	}
	
	va_end(va);
	
	*end = 0;
	
	return out;
}


// concatenate all argument strings together in a new buffer,
//    with the given joining string between them
char* strjoin_(char* joiner, size_t nargs, ...) {
	size_t total = 0;
	char* out, *end;
	size_t j_len;
	
	if(nargs == 0) return NULL;
	
	// calculate total buffer len
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
		if(s) {
			if(i > 0) {
				strcpy(end, joiner);
				end += j_len;
			}
			
			strcpy(end, s); // not exactly the ost efficient, but maybe faster than
			end += strlen(s); // a C version. TODO: test the speed
		};
	}
	
	va_end(va);
	
	*end = 0;
	
	return out;
}


// allocates a new buffer and calls sprintf with it
// why isn't this a standard function?
char* sprintfdup(char* fmt, ...) {
	va_list va;
	
	va_start(va, fmt);
	size_t n = vsnprintf(NULL, 0, fmt, va);
	char* buf = malloc(n + 1);
	va_end(va);
	
	va_start(va, fmt);
	vsnprintf(buf, n + 1, fmt, va);
	va_end(va);
	
	return buf;
}



char** strsplit(char* splitters, char* in, long* out_len) {
	char* e;
	int alloc = 32;
	int len = 0;
	char** list = malloc(alloc * sizeof(*list)); 
	
	for(char* s = in; *s;) {
		e = strpbrk(s, splitters);
		if(!e) e = s + strlen(s);
		
		if(len >= alloc - 1) {
			alloc *= 2;
			list = realloc(list, alloc* sizeof(*list));
		}
		
		list[len++] = strndup(s, e - s);
		
		e += strspn(e, splitters);
		s = e;
	}
	
	list[len] = NULL;
	if(out_len) *out_len = len;
	
	return list;
}

char** read_split_file(char* path, char* sep, long* out_len) {
	int fd;
	struct stat st;
	char* contents;
	
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Could not open file '%s'.\n", path);
		return NULL;
	}
	
	fstat(fd, &st);
	
	contents = mmap(NULL, st.st_size + 1, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(contents == MAP_FAILED) {
		fprintf(stderr, "Failed to map anonymous memory region of size %d\n", st.st_size + 1);
	}
	
	contents = mmap(contents, st.st_size, PROT_READ, MAP_SHARED | MAP_FIXED | MAP_POPULATE, fd, 0);
	if(contents == MAP_FAILED) {
		fprintf(stderr, "Failed to map file into memory: '%s'\n", argv[1]);
	}
	
	char** out = strsplit(sep, contents, out_len);
	
	munmap(contents, st.st_size);
	close(fd);

	return out;
}


