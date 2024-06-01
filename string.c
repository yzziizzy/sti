// Public Domain.

#include <stdarg.h>
#include <stddef.h> // ptrdiff_t
#include <string.h>
#include <stdint.h>
#include <stdio.h> // printf, sprintf, vsnprintf
#include <stdlib.h> // malloc, realloc
#include <ctype.h>

#include "string.h"




static inline int is_x_digit(int c) {
	return (
		('0' <= c && c <= '9') || 
		('a' <= c && c <= 'f') || 
		('A' <= c && c <= 'F') 
	); 
}



// returns a pointer to the found char, or NULL
char* strnchr(const char* s, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == c) return (char*)&s[i];
    }

    return NULL;
}

// returns s
char* strrev(char* s) {
    size_t len = 0;

    while (s[len]) len++;

    char tmp = '\0';

    for (size_t i = 0, back = len - 1; i < len / 2; i++, back--) {
        tmp = s[back];
        s[back] = s[i];
        s[i] = tmp;
    }

    return s;
}

// returns s
char* strnrev(const char* s, size_t n) {
    char tmp = '\0', *result = (char*)s;

    for (size_t i = 0, back = n - 1; i < n / 2; i++, back--) {
        tmp = result[back];
        result[back] = result[i];
        result[i] = tmp;
    }

    return result;
}

// returns a pointer to the first match character
char* strnpbrk(const char* s, const char* accept, size_t n) {
    uint64_t table[4] = {0};

    for (size_t j = 0; accept[j] != '\0'; j++)
        table[accept[j] >> 6] |= 1 << (accept[j] & 63);

    for (size_t i = 0; i < n; i++)
        if (table[s[i] >> 6] & 1 << (s[i] & 63)) return (char*)&s[i];


    return NULL;
}

// returns a pointer to the first match character
char* strrpbrk(const char* s, const char* accept) {
    uint64_t table[4] = {0};

    for (size_t j = 0; accept[j] != '\0'; j++)
        table[accept[j] >> 6] |= 1 << (accept[j] & 63);

    char *result = NULL;

    for (size_t i = 0; s[i] != '\0'; i++)
        if (table[s[i] >> 6] & 1 << (s[i] & 63)) result = (char*)&s[i];

    return result;
}

// returns a pointer to the first match character
char* strnrpbrk(const char* s, const char* accept, size_t n) {
    uint64_t table[4] = {0};

    for (size_t j = 0; accept[j] != '\0'; j++)
        table[accept[j] >> 6] |= 1 << (accept[j] & 63);

    char *result = NULL;

    for (size_t i = 0; i < n; i++)
        if (table[s[i] >> 6] & 1 << (s[i] & 63)) result = (char*)&s[i];

    return result;
}


// The length of the initial part of "s" not containing any of the characters that are part of "reject".
size_t strncspn(const char* s, const char* reject, size_t n) {
    uint64_t table[4] = {0};

    for (size_t j = 0; reject[j] != '\0'; j++)
        table[reject[j] >> 6] |= 1 << (reject[j] & 63);

    size_t i = 0;

    for (; i < n; i++)
        if (table[s[i] >> 6] & 1 << (s[i] & 63)) return i;

    return i;
}

// The length of the initial part of "s" not containing any of the characters that are part of "reject".
size_t strrcspn(const char* s, const char* reject) {
    uint64_t table[4] = {0};

    for (size_t j = 0; reject[j] != '\0'; j++)
        table[reject[j] >> 6] |= 1 << (reject[j] & 63);

    size_t i = 0, count = 0;

    for (; s[i] != '\0'; i++)
        if (table[s[i] >> 6] & 1 << (s[i] & 63)) count = 0;
        else count++;

    return count;
}

// The length of the initial part of "s" not containing any of the characters that are part of "reject".
size_t strnrcspn(const char* s, const char* reject, size_t n) {
    uint8_t v, index;
    uint64_t table[4] = {0};

    for (size_t j = 0; reject[j] != '\0'; j++)
        table[reject[j] >> 6] |= 1 << (reject[j] & 63);

    size_t i = 0, count = 0;

    for (; i < n; i++)
        if (table[s[i] >> 6] & 1 << (s[i] & 63)) count = 0;
        else count++;

    return count;
}

// return the number of characters spanned
size_t strnrspn(const char* s, const char* accept, size_t n) {
    uint64_t table[4] = {0};

    for (size_t j = 0; accept[j] != '\0'; j++)
        table[accept[j] >> 6] |= 1 << (accept[j] & 63);

    size_t i = 0, count = 0;

    for (; i < n; i++)
        if (!(table[s[i] >> 6] & 1 << (s[i] & 63))) count = 0;
        else count++;


    return count;
}

// moves chars to left, returns s
char* strnltrim(char* s, const char* charset, size_t n) {
    uint8_t v, index;
    uint64_t table[4] = {0};

    for (size_t j = 0; charset[j] != '\0'; j++)
        table[charset[j] >> 6] |= 1 << (charset[j] & 63);

    size_t i = 0;

    while (i < n) {
        if (!(table[s[i] >> 6] & 1 << (s[i] & 63))) break;
        i++;
    }

    memmove(s, s + i, n - i);

    return s;
}

// does not trim, returns s
char* strcolwsp(char* s, int c) {
    size_t size = 0;
    uint8_t multiple = 0;

    for (size_t i = 0; s[i] != '\0'; i++) {
        if (isspace(s[i])) {
            if (!multiple) {
                s[size++] = c;
                multiple = 1;
            }
        }
        else {
            s[size++] = s[i];
            multiple = 0;
        }
    }

    s[size] = '\0';

    return s;
}

// does not trim, returns s
char* strncolwsp(char* s, int c, size_t n) {
    size_t size = 0;
    uint8_t multiple = 0;

    for (size_t i = 0; i < n; i++) {
        if (isspace(s[i])) {
            if (!multiple) {
                s[size++] = c;
                multiple = 1;
            }
        }
        else {
            s[size++] = s[i];
            multiple = 0;
        }
    }

    s[size] = '\0';

    return s;
}

// also trims, returns s
char* strcolwsptrim(char* s, int c) {
    size_t size = 0;
    uint8_t multiple = 0;

    for (size_t i = 0; s[i] != '\0'; i++) {
        if (isspace(s[i])) {
            if (!multiple && size > 0) {
                s[size++] = c;
                multiple = 1;
            }
        }
        else {
            s[size++] = s[i];
            multiple = 0;
        }
    }

    if (size > 0)
        if (isspace(s[size - 1]))
            size--;

    s[size] = '\0';

    return s;
}

// capitalize the first letter following whitespace, and the beginning of the string, returns s
char* strcapwords(char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (i == 0) s[i] = toupper(s[i]);
        else if (isspace(s[i])) {
            while (isspace(s[++i]));

            s[i] = toupper(s[i]);
        }
    }

    return s;
}

// capitalize the first letter following whitespace, and the beginning of the string, returns s
char* strncapwords(char* s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (i == 0) s[i] = toupper(s[i]);
        else if (isspace(s[i])) {
            while (isspace(s[++i]));

            s[i] = toupper(s[i]);
        }
    }

    return s;
}

// capitalize the first letter following terminal punctuation, and the beginning of the string, returns s
char* strcapsentences(char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (i == 0) s[i] = toupper(s[i]);
        else if (s[i] == '.' || s[i] == '!' || s[i] == '?') {
            while (isspace(s[++i]));
            s[i] = toupper(s[i]);
        }
    }

    return s;
}

// capitalize the first letter following terminal punctuation, and the beginning of the string, returns s
char* strncapsentences(char* s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (i == 0) s[i] = toupper(s[i]);
        else if (s[i] == '.' || s[i] == '!' || s[i] == '?') {
            while (isspace(s[++i]));
            s[i] = toupper(s[i]);
        }
    }

    return s;
}

// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s) {
	char* n;
	
	n = strchr(s, '\n');
	if(!n) return strlen(s);
	
	return n - s;
}

// strdup a line
char* strlndup(const char* s) {
	return strndup(s, strlnlen(s));
}

// line count;
size_t strlinecnt(const char* s) {
	size_t n;

	if(!*s) return 0;
	
	n = 1;
	while(*s) // just to make you cringe
		if(*s++ == '\n') 
			n++;
	
	return n;
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



// returns a null-terminated list of pointers to each line.
// mutates the source (replaces newlines with nulls)
char** strsplit_inplace(char* src, char delim, size_t* outLen) {
	size_t alloc = 8;
	size_t len = 0;
	char** out = malloc(alloc * sizeof(*out));
	
	char* start = src;
	size_t i;
	
	for(i = 0; src[i] != 0; i++) {
		if(src[i] == delim) {
			src[i] = 0; // put in a null terminator
			
			
			out[len++] = start;
			start = src + i + 1;
			
			// always have two extra for the end
			if(len + 1 >= alloc){
				alloc *= 2;
				out = realloc(out, alloc * sizeof(*out));
			}
			
		}
	}
	
	if(src + i != start) {
		out[len++] = start;
	}
	
	out[len] = NULL;
	
	if(outLen) *outLen = len;
	
	return out;
}


char** strsplit(char* src, char delim, size_t* outLen) {
	size_t alloc = 8;
	size_t len = 0;
	char** out = malloc(alloc * sizeof(*out));
	
	size_t start_i = 0;
	size_t i;
	
	for(i = 0; src[i] != 0; i++) {
		if(src[i] == delim) {
			
			out[len++] = strndup(src + start_i, i - start_i);
			start_i = i + 1;
			
			// always have two extra for the end
			if(len + 1 >= alloc){
				alloc *= 2;
				out = realloc(out, alloc * sizeof(*out));
			}
			
		}
	}
	
	if(i >= start_i) {
		out[len++] = strdup(src + start_i);
	}
	
	out[len] = NULL;
	
	if(outLen) *outLen = len;
	
	return out;
}


size_t strnspn(const char* s, size_t count, const char* accept) {
	const char* e = s;
	
	for(size_t i = 0; i < count && NULL != strchr(accept, *e); i++, e++);
	
	return e - s;
}



size_t strrspn(const char* s, const char* accept) {
	const char* e, *r;
	
	e = s;
	while(*e) e++;
	
	r = e - 1;
	while(r >= s && NULL != strchr(accept, *r)) 
		r--;
	
	return e - r - 1;
}


const char* strrstr(const char* haystack, const char* needle) {
	const char* best = NULL;
	
	for(const char* p = haystack; *p; p++) {
		for(long i = 0; ; i++) {
			if(needle[i] == 0) {
				best = p;
				break;
			}
				
			if(needle[i] != p[i] || !p[i]) {
				break;
			}
		}
	}
	
	return best;
}


size_t strtriml(char* s, const char* trim) {
	size_t n, l;
	
	n = strspn(s, trim);
	l = strlen(s + n);
	memmove(s, s + n, l + 1);
	
	return l;
}

size_t strtrimr(char* s, const char* trim) {
	size_t n = strrspn(s, trim);
	s[strlen(s) - n] = 0;
	
	return n;
}

// left and right
size_t strtrim(char* s, const char* trim) {
	size_t n;
	
	strtriml(s, trim);
	n = strtrimr(s, trim);
	
	return n;
}



unsigned int decodeHexDigit(char c) {
	if(c >= '0' && c <= '9') {
		return c - '0';
	}
	else if(c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}
	else if(c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}
	return 0;
}
/*
static double nibbleHexNorm(char* s) {
	if(s[0] == '\0' || s[1] == '\0') return 0.0;
	double d = (decodeHexDigit(s[0]) * 16.0) + decodeHexDigit(s[1]);
	return d / 256.0;
}
*/

// returns rgba, with r in most significant bits and a in the least
uint32_t decodeHexColor(char* s) {
	int i;
	unsigned short c[4] = {0,0,0,255};
	
	// throw away any leading BS
	char* e = s + strlen(s);
	if(s[0] == '#') s++;
	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
	
	// the actual decoding
	for(i = 0; i < 4 && s < e; i++) {
		c[i] = (decodeHexDigit(s[0]) << 4) + decodeHexDigit(s[1]);
		c[i] = c[i] > 255 ? 255 : c[i];
		s += 2;
	}
	
// 	printf(" color: %d,%d,%d,%d\n", c[0], c[1], c[2], c[3]);
	
	uint32_t o = 
		(((uint32_t)c[0]) << 24) |
		(((uint32_t)c[1]) << 16) |
		(((uint32_t)c[2]) << 8) |
		(((uint32_t)c[3]) << 0);
	
	return o;
}


void decodeHexColorNorm(char* s, float* out) {
	int i;
	out[0] = 0.0;
	out[1] = 0.0;
	out[2] = 0.0;
	out[3] = 1.0;
	
	// throw away any leading BS
	char* e = s + strlen(s);
	if(s[0] == '#') s++;
	if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
	
	// the actual decoding
	for(i = 0; i < 4 && s < e; i++) {
		int n = (decodeHexDigit(s[0]) << 4) + decodeHexDigit(s[1]);
		out[i] = n / 255.0; 
		s += 2;
	}
}




// decodes strings according to the string literal rules in C
// *s is advanced to the next char
// gleefully advances the pointer through nulls like any other character
// returns 1 if the character was escaped 
// returns an error code on invalid escape sequences
int decode_c_string_char(char** s, int* c_out) {
	int acc = 0;
	int max = 4;
	int i;
	
	int c = **s;
	(*s)++;
		
	if(c != '\\') {
		*c_out = c;
		return 0;
	}

	// escape sequence of some kind
	
	c = **s;
	(*s)++;
	
	switch(c) {
		case 'a': *c_out = 0x07; break;
		case 'b': *c_out = 0x08; break;
		case 'e': *c_out = 0x1b; break;
		case 'f': *c_out = 0x0c; break;
		case 'n': *c_out = 0x0a; break;
		case 'r': *c_out = 0x0d; break;
		case 't': *c_out = 0x09; break;
		case 'v': *c_out = 0x0b; break;
		case '\\': *c_out = 0x5c; break;
		case '\'': *c_out = 0x27; break;
		case '"': *c_out = 0x22; break;
		case '?': *c_out = 0x3f; break;
		
		case '0': // \nnn octal
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': 
			for(i = 0; i < 3; i++) {
				acc = (acc << 3) + c - '0';
				
				c = **s;
				(*s)++;
				
				if('0' > c || c > '9') break;
			}
			
			*c_out = acc;
			return 1;
		
		case 'x': // \xnn... hex sequence
			for(i = 0;; i++) {
				
				c = **s;
				(*s)++;
				
				if(!is_x_digit(c)) break;
				
				acc = (acc << 4) + decodeHexDigit(c);
			}
			
			*c_out = acc;
			return i >= 1 ? 1 : -1;
		
		
		case 'U': // \Unnnnnnnn
			max = 8;
			/* fallthrough */
			
		case 'u': // \unnnn hex
			for(i = 0; i < max; i++) {
				c = **s;
				(*s)++;
				
				if(!is_x_digit(c)) break;
				
				acc = (acc << 4) + decodeHexDigit(c);
			}
			
			*c_out = acc;
			return i == max ? 1 : -1;
			
		default:
			*c_out = c;
			return -2;
	}
	
	
	return 1;
}



int is_number_char(int c) {
	switch(c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':	
		case '5':	
		case '6':	
		case '7':	
		case '8':	
		case '9':	
		case '-':	
		case '.':	
		case 'e':	
		case 'E':	
		case 'x':	
		case 'X':	
			return 1;
	}
	
	return 0;
}






int read_c_number(char** s, number_parse_info* info) {
	char* s2 = *s;
	
	// probe the base
	if(s2[0] == '0') {
		if(s2[1] == 'x' || s2[1] == 'X') {
			info->base = 16;
		}
		if(s2[1] == 'b' || s2[1] == 'B') {
			info->base = 2;
		}
		else {
			info->base = 8;
		}
		
		s2 += 2;
	}
	else {
		info->base = 10;
	}
	

	// probe if it's a float or an integer type so the right fn can be called
	// skip past numbers until we find an indicator
	char type = 'i';
	
	for(int i = 0; s2[i]; i++) {
		if(isxdigit(s2[i])) continue;
		
		switch(s2[i]) {
			case '.':
			case 'e':
			case 'E':
				type = 'f';
				break;
				
			case 'p':
			case 'P':
				if(info->base == 16) {
					type = 'f';
				}
				break;
		}
		
		break;
	}
	
	
	// convert the numeric part
	if(type == 'i') {
		info->n = strtoull(*s, &s2, info->base);
	}
	else {
		info->f = strtold(*s, &s2);
	}
	
	// check for suffixes
	while(*s2) {
		switch(s2[0]) {
			case 'u':
			case 'U':
				info->not_signed = 'u';
				s2++;
				continue;
				
			case 'l':
			case 'L':
				info->longs++;
				s2++;
				continue;
		}
		
		break;
	}
	
	*s = s2;
	
	return 0;
}






/*
char** sane_prefixes[] = {
	"B",
	"KB",
	"MB",
	"GB",
	"TB",
	"PB",
	"EB",
	"ZB",
	"YB",
};

void format_bytes(char* buf, size_t buf_len, uint64_t bytes, int sig_figs) {
	
	// find the greatest two sections
	uint64_t n = bytes;
	uint64_t old = 0;
	int oom = 0;
	
	while(1) {
		if(n < 1024) break;
		
		old = n;
		n /= 1024;
	}
	
	old -= n * 1024;
	
	
	

}
*/




/*
Returns the reversed string

Garbage in, garbage out.
Unsigned ints only.
Dont't give a base of 1.
Dont't give a base greater than 36.
Give a sufficiently long buffer. You figure it out first.
Give a char set as long as your base.
*/
/*static*/ int uint_r_cvt_str(uint64_t n, int base, char* buf, char* charset) {
	
	int i = 0;
	
	while(n > 0) {
		int64_t b = n / base;
		int     a = n % base;
		
		buf[i++] = charset[a];
		
		n = b;
	}
	
	return i;
}

/*
Does not add a minus character 
Garbage in, garbage out. 
Dont't give a base of 1.
Dont't give a base greater than 36.
Give a sufficiently long buffer. You figure it out first.
Give a char set as long as your base.
*/
/*static*/ int int_r_cvt_str(int64_t n, int base, char* buf, char* charset) {
// 	char negative = 0;
	int i = 0;
	
	if(n < 0) {
// 		negative = 1;
		n = -n;
	}
	
	i = uint_r_cvt_str(n, base, buf, charset);
	
// 	if(negative) buf[i++] = '-';
	// GPUEDIT: reformat all numbers in selection by fmt string
	return i;
}

/*static*/ int flt_r_cvt_str(float f, int base, char* buf, char* charset) {
	int i = 0;
	
	union {
		float f;
		uint32_t u;
	} u;
	
	u.f = f;
	uint32_t nf = u.u; 
	
	if(nf == 0x7f800000) { // infinity
		strcpy(buf, "infinity");
		return strlen("infinity");
	}
	if(nf == 0xff800000) { // -infinity
		strcpy(buf, "-infinity");
		return strlen("-infinity");
	}
	
	uint32_t sign =  (nf & 0x80000000) >> 31;
	uint32_t exp_r = (nf & 0x7ff00000) >> 20;
	uint32_t frac =  (nf & 0x000fffff) | 0x00100000;
	int32_t exp = exp_r - 127;
	
	printf("%x, %x, %x\n", sign, exp_r, frac);
	
	(void)base;
	(void)buf;
	(void)charset;
	(void)exp;
	
	
	
	while(nf > 0) {
 		int64_t b = nf / base;
 		int     a = nf % base;
 		
 		buf[i++] = charset[a];
 		
 		nf = b;
 	}
	
// 	if(negative) buf[i++] = '-';
	
	return i;
}

/*
Garbage in, garbage out. 
Dont't give a base of 1.
Dont't give a base greater than 36.
Give a sufficiently long buffer. You figure it out first.
*/
/*static*/ int int_r_cvt(int64_t n, int base, int upper, char* buf) {
	
	if(n == 0) {
		buf[0] = '0';
		return 1;
	}
	
	static char* lchars = "0123456789abcdefghijklmnopqrstuvwxyz"; 
	static char* uchars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	return int_r_cvt_str(n, base, buf, upper ? uchars : lchars);
}
/*
Garbage in, garbage out. 
Dont't give a base of 1.
Dont't give a base greater than 36.
Give a sufficiently long buffer. You figure it out first.
*/
/*static*/ int uint_r_cvt(uint64_t n, int base, int upper, char* buf) {
	
	if(n == 0) {
		buf[0] = '0';
		return 1;
	}
	
	static char* lchars = "0123456789abcdefghijklmnopqrstuvwxyz"; 
	static char* uchars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	return uint_r_cvt_str(n, base, buf, upper ? uchars : lchars);
}


int sprintlongb(char* buf, int base, int64_t n, char* charset) {
	char negative = 0;
	int o = 0;
	
	if(n < 0) {
		negative = 1;
		n = -n;
	}
	
	while(n > 0) {
		int64_t b = n / base;
		int     a = n % base;
		
		buf[o++] = charset[a];
		
		n = b;
	}
	
	if(negative) buf[o++] = '-';
	
	int limit = o / 2;
	char tmp;
	
	int i;
	for(i=0;i<limit;i++) {
		tmp = buf[o-i-1];
		buf[o-i-1] = buf[i];
		buf[i] = tmp;
	}
	buf[o] = '\0';
	
	return o;
}



int int_r_add_commas(char* buf, int len) {
	char tmp[64];
	int n = 0;
	
	strncpy(tmp, buf, len);
	
	for(int i = 0; i < len; n++, i++) {
		if(i && i % 3 == 0) {
			buf[n++] = ',';
		}
		buf[n] = tmp[i];
	}
	
	return n;
}



int iprintf(char* fmt, ...) {
	(void)fmt;
#if 0
	va_list va;
	
	va_start(va, fmt);
	int n = 0;
	
	for(size_t i = 0; fmt[i]; i++) { 
		
		
		char c = fmt[i];
		if(c == '%') {
			// %[flags][width][.precision][length]specifier
			
			int64_t i64;
			uint64_t u64;
			char* str;
// 			double dbl;
			
			int len;
			
			int starti = i;
			char* start = fmt + i;
			char fmtbuf[16];
			char buf[64];
			
			char uppercase = 0;
			char add_commas = 0;
			char force_sign = 0;
// 			char left_justify = 0;
// 			char optional_sign = 0;
// 			char force_decimal = 0;
// 			char pad_zeroes = 0;
// 			char vector_print = 0;
			
// 			int width = 0;
// 			int precision = 0;
			
			c = fmt[++i];
			
			if(c == 0) break;
			if(c == '%') {
				putc('%', stdout); 
				continue;
			}
			
			// flags
			while(1) {
				if(c == 0) { // end of fmt string
					goto END_STR;
				}
				else if(c == ',') { // add thousands separators
					add_commas = 1;
					c = fmt[++i];
				}
				else if(c == '-') { // left-justify
					c = fmt[++i];
				}
				else if(c == '+') { // show sign
					force_sign = 1;
					c = fmt[++i];
				}
				else if(c == ' ') { // optional sign
					c = fmt[++i];
				}
				else if(c == '#') { // force decimal point
					c = fmt[++i];
				}
				else if(c == '0') { // left-pad zeroes
					c = fmt[++i];
				}
				else if(c == 'v') { // vector
					c = fmt[++i];
				}
				else break;
			}
			
			// width
			if(c == '*') { // specified in args
				i++;
				// va_arg(va, n);
				n++;
			}
			else if(c >= '0' && c <= '9') {
				while(c >= '0' && c <= '9') {
					c = fmt[i++];
					if(c == 0) goto END_STR;
				}
			}
			
			// precision
			if(c == '.') {
				c = fmt[++i];
			
				if(c == 0) goto END_STR;
				else if(c == '*') { // specified in args
					n++;
					// va_arg(va, n);
				}
				else {
					while(c >= '0' && c <= '9') {
						c = fmt[i++];
						if(c == 0) goto END_STR;
					}
				}
			}
			
			// length 
			if(c == 0) goto END_STR;
			else if(c == 'h') { // short int // hh = char
				c = fmt[i++];
			}
			else if(c == 'l') { // long // ll = long long
				c = fmt[i++];
			}
			else if(c == 'z') { // size_t
				c = fmt[i++];
			}
			else if(c == 't') { // ptrdiff_t
				c = fmt[i++];
			}
			else if(c == 'j') { // uintmax_t
				c = fmt[i++];
			}
			else if(c == 'L') { // long double
				c = fmt[i++];
			}
			
			// specifier
			switch(c) {
				case 0: goto END_STR;
				
				case 'b': // binary
					u64 = va_arg(va, uint64_t);
					
					len = uint_r_cvt(u64, 2, uppercase, buf);
					
					for(int i = len - 1; i >=0; i--) {
						putc(buf[i], stdout);
					} 
					
					break;
					
				case 'd': 
				case 'i': // signed decimal int
					i64 = va_arg(va, int64_t);
					
					len = int_r_cvt(i64, 10, uppercase, buf);
					
					if(add_commas) {
						// causes an annoying warning i don't feel like dealing with now
//						len = int_r_add_commas(buf, len);
					}
					
					if(force_sign && i64 > 0) putc('+', stdout);
					if(i64 < 0) putc('-', stdout);
					
					for(int i = len - 1; i >=0; i--) {
						putc(buf[i], stdout);
					}
				
					
					break;
				
				case 'u': // unsigned decimal int
					u64 = va_arg(va, uint64_t);
					
					len = uint_r_cvt(u64, 10, uppercase, buf);
					
					if(add_commas) {
					// causes an annoying warning i don't feel like dealing with now
//						len = int_r_add_commas(buf, len);
					}
					
					if(force_sign) putc('+', stdout);
					
					for(int i = len - 1; i >=0; i--) {
						putc(buf[i], stdout);
					} 
					break;
				
				case 'o': // unsigned octal int 
					u64 = va_arg(va, uint64_t);
					
					len = uint_r_cvt(u64, 8, uppercase, buf);
					
					if(force_sign) putc('+', stdout);
					
					for(int i = len - 1; i >=0; i--) {
						putc(buf[i], stdout);
					} 
					break;
				
				case 'X': uppercase = 1; /* fallthrough */
				case 'x': // unsigned hex int
					u64 = va_arg(va, uint64_t);
					
					len = uint_r_cvt(u64, 16, uppercase, buf);

					if(force_sign) putc('+', stdout);
					
					for(int i = len - 1; i >=0; i--) {
						putc(buf[i], stdout);
					} 
					break;
				
				// printing floats is very complicated.
				// fall back to printf, because custom float printing is
				//   not the point of iprintf()
				case 'F': uppercase = 1; /* fallthrough */
// 					break;
				case 'E': uppercase = 1;  /* fallthrough */
// 					break; /* fallthrough */
				case 'G': uppercase = 1;  /* fallthrough */
// 					break; /* fallthrough */
				case 'A': uppercase = 1;  /* fallthrough */
				case 'f': // float, decimal
				case 'e': // decimal scientific notation
				case 'g': // shortest of e/f
				case 'a': // hex float
					strncpy(fmtbuf, start, i - starti);
					sprintf(buf, fmtbuf, va_arg(va, double));
					
					puts(buf);
					
					break;
				
				case 'c': // character
					putc(va_arg(va, int), stdout);
					break;
				
				case 's': // string
					str = va_arg(va, char*);
					fputs(str, stdout);
					break;
					
				case 'P': // shortened pointer
				case 'p': // pointer
					u64 = va_arg(va, uint64_t);
					
					len = uint_r_cvt(u64, 16, 0, buf);
					fputs("0x", stdout);
					
					for(int i = len - 1; i >=0; i--) {
						putc(buf[i], stdout);
					} 
					break;
					
				case 'n': // set arg to number of chars written
					break;
					
				default: 
					break;
			}
			
			(void)uppercase;
			
		}
		else {
			putc(c, stdout);
		}
		

		
	}
	
END_STR:
	
	va_end(va);
	
#endif
	return 0;
}

/*
static void indirect(int amt, int index, void** a, void** b, void** c) {
	if(index == 0) {
		while(amt-- > 0) *a = *((void**)(*a));
	}
	else if(index == 1) {
		while(amt-- > 0) *b = *((void**)(*b));
	}
	else {
		while(amt-- > 0) *c = *((void**)(*c));
	}
}
*/

int isnprintfv(char* out, ptrdiff_t out_sz, char* fmt, void** args) {
	(void)out;
	(void)out_sz;
	(void)fmt;
	(void)args;
#if 0
	int n = 0;
	int ar = 0;
	
	char fmt_buf[64];
	fmt_buf[0] = '%';
	
	
	for(size_t i = 0; fmt[i];) { 
		char c = fmt[i];
// 		printf("+i:%ld+", i);
		if(c == '%') {
			
			int Indirect = 0;
			
			char Int = 0;
			char Double = 0;
			char Pointer = 0;
// 			char Broken = 0;
			
			char WidthStar = 0;
// 			char WidthDollar = 0;
			char PrecStar = 0;
// 			char PrecDollar = 0;
			char Long = 0;
			char Half = 0;
			
			int fblen = 1;
			
			int numExtra = 0;
			
			c = fmt[++i];
			
			if(c == 0) break;
			if(c == '%') {
				if(n < out_sz - 1) out[n] = '%';
				i++;
				n++;
				continue;
			}
			
			// flags
			while(1) {
				switch(c) {
					default:
						goto END_FLAGS;
						
					case '#':
					case '0':
					case '-':
					case ' ':
					case '+':
					case '\'':
					case 'I':
						fmt_buf[fblen++] = c;
						c = fmt[++i];
						continue;
					case '>':
						Indirect++;
						c = fmt[++i];
						continue;
				}
			}
		END_FLAGS:
			
			// field width
			if(c == '*') {
				WidthStar = 1;
				fmt_buf[fblen++] = c;
				c = fmt[++i];
			}
			
			while(1) {
				if(c < '0' || c > '9') break;
				
				fmt_buf[fblen++] = c;
				c = fmt[++i];
			}
			
			if(WidthStar && c == '$') {
// 				WidthDollar = 1;
				fmt_buf[fblen++] = c;
				c = fmt[++i];
			}
			
			// precision
			if(c == '.') {
				fmt_buf[fblen++] = c;
				c = fmt[++i];
				
				if(c == '*') {
					PrecStar = 1;
					fmt_buf[fblen++] = c;
					c = fmt[++i];
				}
				
				while(1) {
					if(c < '0' || c > '9') break;
					
					fmt_buf[fblen++] = c;
					c = fmt[++i];
				}
				
				if(c == '$') {
// 					PrecDollar = 1;
					fmt_buf[fblen++] = c;
					c = fmt[++i];
				}
			}
			
			// length
			
			switch(c) {
				case 'h':
					Half = 1;
					goto LENGTH_NEXT;
					
				case 'l':
				case 'L':
				case 'z':
				case 'Z':
				case 't':
					Long = 1; /* fallthrough */
// 				case 'hh':
// 				case 'll':
				case 'q':
				case 'j':
				LENGTH_NEXT:
					fmt_buf[fblen++] = c;
					c = fmt[++i];
			}
			
			// conversion specifier
			switch(c) {
				case 'C':
					Long = 1; /* fallthrough */
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
				case 'c':
					Int = 1;
					break;
					
				case 'e':
				case 'E':
				case 'f':
				case 'F':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					Double = 1;
					break;
					
				case 's':
				case 'S':
				case 'p':
					Pointer = 1;
					break;
					
				case 'n':
					*((uint64_t*)args[ar]) = n;
					ar++;
					i++;
					continue;
					
// 				case 'm': // not supported
				
				default:
// 					Broken = 1;
					fprintf(stderr, "Broken format specifier.\n");
					i++;
					continue;
					// broken specifier
			}
			
			
			fmt_buf[fblen++] = c;
			fmt_buf[fblen] = 0;
			c = fmt[++i];
			
// 			printf("(%s)", fmt_buf);
			
			if(PrecStar) numExtra++;
			if(WidthStar) numExtra++;
			
			void* a = args[ar];
			void* b = args[ar+1];
			void* c = args[ar+2];
			
			indirect(Indirect, numExtra, &a, &b, &c);
			/*
			if(Double) {
				if(Half) { // yes, backwards, but compatible with standard format strings
					if(numExtra == 0) n += snprintf(out + n, out_sz - n, fmt_buf, *((float*)&a));
					else if(numExtra == 1) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, *((float*)&b));
					else if(numExtra == 2) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint64_t)b, *((float*)&c));
				}
				else {
					if(numExtra == 0) n += snprintf(out + n, out_sz - n, fmt_buf, *((double*)&a));
					else if(numExtra == 1) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, *((double*)&b));
					else if(numExtra == 2) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint64_t)b, *((double*)&c));
				}
			}
			else if(Pointer) {
				if(numExtra == 0) n += snprintf(out + n, out_sz - n, fmt_buf, (void*)a);
				else if(numExtra == 1) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (void*)b);
				else if(numExtra == 2) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint64_t)b, (void*)c);
			}
			else if(Int) {
				
				if(Long) {
					if(numExtra == 0) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a);
					else if(numExtra == 1) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint64_t)b);
					else if(numExtra == 2) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint64_t)b, (uint64_t)c);
				}
				else { // careful of precision
					// BUG fix int cast 
					if(numExtra == 0) n += snprintf(out + n, out_sz - n, fmt_buf, (uint32_t)(uint64_t)a);
					else if(numExtra == 1) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint32_t)(uint64_t)b);
					else if(numExtra == 2) n += snprintf(out + n, out_sz - n, fmt_buf, (uint64_t)a, (uint64_t)b, (uint32_t)(uint64_t)c);
				}
			}
			*/
// 			printf("-ex:%d-", numExtra);
			ar += 1 + numExtra;
			
		}
		else {
			if(n < out_sz - 1) out[n] = c;
			n++;
			i++;
		}
	}
	
	out[out_sz < n ? out_sz : n] = 0;
	
	return n;
#endif
	return 0;
}





#ifdef STI_REPLACE_STD_STRING

#if !__has_builtin(__builtin_memcpy)
void* memcpy(void* restrict dst, const void *restrict src, size_t n) {
	void* d = dst;
	while(n--) *dst++ = *src++;
	return d;
}	
#endif

#if !__has_builtin(__builtin_memset)
void* memset(void* s, int c, size_t n) {
	while(n--) ((unsigned char*)s)[n] = c;
	return s;
}
#endif


#endif // STI_REPLACE_STD_STRING
