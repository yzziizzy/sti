// Public Domain.

#include <stdarg.h>
#include <string.h>

#include "string.h"





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



// append b to a in a new buffer
char* strappend(const char* a, const char* const b) {
	if(a == NULL) return strdup(b);
	if(b == NULL) return strdup(a);
	
	size_t la = strlen(a);
	size_t lb = strlen(b);
	char* o = malloc(la + lb + 1);
	strcpy(o, a);
	strcpy(o + la, b);
	o[la + lb] = '\0';
	return o;
}



// returns a null-terminated list of pointers to each line.
// mutates the source (replaces newlines with nulls)
char** strsplit_inplace(char* src, char delim, size_t* outLen) {
	size_t alloc = 8;
	size_t len = 0;
	char** out = malloc(alloc * sizeof(*out));
	
	char* start = src;
	
	
	for(size_t i = 0; src[i] != 0; i++) {
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
	
// 	out[len++] = start;
	out[len++] = NULL;
	
	if(outLen) *outLen = len;
	
	return out;
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




/*
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
Garbage in, garbage out. 
Dont't give a base of 1.
Dont't give a base greater than 36.
Give a sufficiently long buffer. You figure it out first.
Give a char set as long as your base.
*/
/*static*/ int int_r_cvt_str(int64_t n, int base, char* buf, char* charset) {
	char negative = 0;
	int i = 0;
	
	if(n < 0) {
		negative = 1;
		n = -n;
	}
	
	i = uint_r_cvt_str(n, base, buf, charset);
	
	if(negative) buf[i++] = '-';
	
	return i;
}

/*static*/ int flt_r_cvt_str(float f, int base, char* buf, char* charset) {
	int i = 0;
	
	uint32_t nf = *((uint32_t*)&f); 
	
	if(nf == 0x7f800000) { // infinity
		strncpy(buf, "infinity", strlen("infinity"));
		return strlen("infinity");
	}
	if(nf == 0xff800000) { // -infinity
		strncpy(buf, "infinity", strlen("infinity"));
		return strlen("infinity") + 1;
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
	
	
	
// 	while(n > 0) {
// 		int64_t b = n / base;
// 		int     a = n % base;
// 		
// 		buf[i++] = charset[a];
// 		
// 		n = b;
// 	}
	
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


int iprintf(char* fmt, ...) {
	va_list va;
	
	va_start(va, fmt);
	int n = 0;
	
	for(size_t i = 0; fmt[i]; i++) { 
		
		
		char c = fmt[i];
		if(c == '%') {
			// %[flags][width][.precision][length]specifier
			
			int starti = i;
			char* start = fmt + i;
			char fmtbuf[16];
			char buf[64];
			
			char uppercase = 0;
// 			char left_justify = 0;
// 			char force_sign = 0;
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
				else if(c == '-') { // left-justify
					c = fmt[++i];
				}
				else if(c == '+') { // show sign
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
			else if(c == 'h') {
				c = fmt[i++];
			}
			else if(c == 'l') {
				c = fmt[i++];
			}
			else if(c == 'z') {
				c = fmt[i++];
			}
			else if(c == 't') {
				c = fmt[i++];
			}
			else if(c == 'j') {
				c = fmt[i++];
			}
			else if(c == 'L') {
				c = fmt[i++];
			}
			
			// specifier
			switch(c) {
				case 0: goto END_STR;
				
				case 'd': 
				case 'i': // signed decimal int
					break;
				
				case 'u': // unsigned decimal int
					break;
				
				case 'o': // unsigned octal int 
					
					break;
				
				case 'X': uppercase = 1; /* fallthrough */
				case 'x': // unsigned hex int
					break;
				
				// printing floats is very complicated.
				// fall back to printf, because custom float printing is
				//   not the point of iprintf()
				case 'F': uppercase = 1; /* fallthrough */
				case 'f': // float, decimal
// 					break;
				case 'E': uppercase = 1;  /* fallthrough */
				case 'e': // decimal scientific notation
// 					break; /* fallthrough */
				case 'G': uppercase = 1;  /* fallthrough */
				case 'g': // shortest of e/f
// 					break; /* fallthrough */
				case 'A': uppercase = 1;  /* fallthrough */
				case 'a': // hex float
					strncpy(fmtbuf, start, i - starti);
					sprintf(buf, fmtbuf, va_arg(va, double));
					
					puts(buf);
					
					break;
				
				case 'c': // character 
					break;
				
				case 's': // string
					break;
					
				case 'P': // shortened pointer
				case 'p': // pointer
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
	
	return 0;
}
