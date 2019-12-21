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
void sexp_free(sexp* x);

int64_t sexp_asInt(sexp* x); 
double sexp_asDouble(sexp* x); 

int64_t sexp_argAsInt(sexp* x, size_t argn);
double sexp_argAsDouble(sexp* x, size_t argn);

// returns internally managed string, user must dup
char* sexp_argAsStr(sexp* x, size_t argn); 
sexp* sexp_argAsSexp(sexp* x, size_t argn); 




#endif // __sti__sexp_h__
