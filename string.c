// Public Domain.

#include <stdarg.h>
#include <string.h>
#include <stdint.h>

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
						len = int_r_add_commas(buf, len);
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
						len = int_r_add_commas(buf, len);
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
	
	return 0;
}


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


int isnprintfv(char* out, ptrdiff_t out_sz, char* fmt, void** args) {
	
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
}
