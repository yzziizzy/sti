#ifndef __sti__sexp_h__
#define __sti__sexp_h__

// Public Domain.

#include <stdint.h>

#include "vec.h"


// A simple S-Expression parser
// Supports (, [, {, and < as brace characters



typedef struct sexp {
	char type; // 0 = list, 1 = literal
	char brace;
	char* str;
	VEC(struct sexp*) args;
	
} sexp;




sexp* sexp_parse(char* source);
sexp* sexp_parse_file(char* path);
void sexp_free(sexp* x);

void sexp_print(int fd, sexp* x);



// if called on a scalar value, returns the requested conversion and ignores argn
// OOB and NULL input tolerant; returns 0/NULL

sexp* sexp_arg(sexp* x, size_t argn);
int64_t sexp_int(sexp* x, size_t argn);
uint64_t sexp_uint(sexp* x, size_t argn);
float sexp_float(sexp* x, size_t argn);
double sexp_double(sexp* x, size_t argn);

// returns internally managed string, user must dup
char* sexp_str(sexp* x, size_t argn);
 





#endif // __sti__sexp_h__
