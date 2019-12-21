// Public Domain.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#include "sexp.h"
#include "fs.h"


static char decode(char** s);



static sexp* parse_literal(char** s) {
	
	sexp* x;
	char* e;
	char q;
	int len, i;
	
	x = calloc(1, sizeof(*x));
	x->type = 1;
	
	//check if it's not quoted
	if(**s != '"' && **s != '\'') {
		
		e = strpbrk(*s, " \r\n\t)");
		if(!e) {
			fprintf(stderr, "sexp: unexpected end of input parsing literal\n");
			return x;
		}
		
		x->str = strndup(*s, e - *s);
		*s = e;
		return x;
	}
	
	// handled quoted strings
	q = **s;
	(*s)++;
	
	// find max length
	for(len = 0; (*s)[len] && (*s)[len] != q && (*s)[len - 1] != '\\'; len++);
	
	x->str = malloc(len + 1);
	
	i = 0;
	while(**s && **s != q && *(*s - 1) != '\\') {
		x->str[i] = decode(s);
		(*s)++;
		i++;
	}
	
	x->str[i] = '\0';
	
	return x;
}



static sexp* parse(char** s) {
	
	sexp* x, *y;
	
	x = calloc(1, sizeof(*x));
	x->type = 0;
	
	while(**s) {
		char c = **s;
		switch(c) {
			case '(': /* fall through */ // sub expression
			case '{': /* fall through */ // sub expression
			case '[': /* fall through */ // sub expression
			case '<': /* fall through */ // sub expression
				(*s)++;
				
				// TODO: check for (*   *) and skip as comment
				
				y = parse(s);
				y->brace = c;
				VEC_PUSH(&x->args, y);
				break;
				
			case ')': /* fall through */ // end of expression
			case '}': /* fall through */ // end of expression
			case ']': /* fall through */ // end of expression
			case '>': /* fall through */ // end of expression
				(*s)++;
				return x;
			
			case '\r': /* fall through */ // skip whitespace
			case '\n': /* fall through */ 
			case '\t': /* fall through */ 
			case '\v': /* fall through */ 
			case ' ':
				(*s)++;
				break;
				
			default: // some literal of some sort
				y = parse_literal(s);
				VEC_PUSH(&x->args, y);
				break;
		}
	}
	
	fprintf(stderr, "sexp: unexpected end of input parsing expression.\n");
	
	return x;
}



sexp* sexp_parse(char* source) {
	char* s = strpbrk(source, "({[<") + 1;
	
	return parse(&s);
}

sexp* sexp_parse_file(char* path) {
	char* s;
	
	s = readWholeFile(path, NULL);
	
	return parse(&s);
}



// returns 0 on all failed conversions
int64_t sexp_asInt(sexp* x) {
	int64_t n;
	int base = 10;
	
	if(!x->str) return 0;
	if(x->type == 0) return 0;
	
	// HACK. does not allow negative hex/octal/binary
	if(x->str[0] == '0') {
		if(x->str[1] == 'x') { // safe, implied by [0] being not null above
			base = 16;
		}
		else if(x->str[1] == 'b') {
			base = 2;
		}
		else {
			base = 8;
		}
	}
	
	n = strtol(x->str, NULL, base);
	
	return n;
} 


double sexp_asDouble(sexp* x) {
	
	if(!x->str) return 0.0;
	if(x->type == 0) return 0.0;

	return strtod(x->str, NULL);
}

int64_t sexp_argAsInt(sexp* x, size_t argn) {
	if(x->type != 0) return 0;
	if(VEC_LEN(&x->args) < argn) return 0;
	 
	return sexp_asInt(VEC_ITEM(&x->args, argn));
}

double sexp_argAsDouble(sexp* x, size_t argn) {
	if(x->type != 0) return 0.0;
	if(VEC_LEN(&x->args) < argn) return 0.0;
	 
	return sexp_asDouble(VEC_ITEM(&x->args, argn));
}

// returns internally managed string, user must dup
char* sexp_argAsStr(sexp* x, size_t argn) {
	if(x->type != 0) return "";
	if(VEC_LEN(&x->args) < argn) return "";
	 
	return VEC_ITEM(&x->args, argn)->str;
}

sexp* sexp_argAsSexp(sexp* x, size_t argn) {
	if(x->type != 0) return NULL;
	if(VEC_LEN(&x->args) < argn) return NULL;
		 
	return VEC_ITEM(&x->args, argn);
}


void sexp_free(sexp* x) {
	size_t i;
	
	if(!x) return;
	
	for(i = 0; i < VEC_LEN(&x->args); i++) {
		sexp_free(VEC_ITEM(&x->args, i));
	}

	if(x->str) free(x->str);
	free(x);
}


























// out must be big enough, at least as big as in+1 just to be safe
// appends a null to out, but is also null-safe
static char decode(char** s) {

	char c = **s;
	
	if(c == '\\') {
		(*s)++;
		switch(**s) {
			case '\'': return '\'';  
			case '"': return '"';  
			case '`': return '`';  
			case '?': return '?';  
			case '0': return '\0';  
			case 'r': return '\r';  
			case 'n': return '\n'; 
			case 'f': return '\f'; 
			case 'a': return '\a'; 
			case 'b': return '\b'; 
			case 'v': return '\v'; 
			case 't': return '\t'; 
			case 'x': 
				// TODO: parse hex code
				return '?';
			case 'U':
				// TODO parse longer unicode
			case 'u': 
				// TODO: parse unicode
				return '?';
			// TODO: parse octal
				
			default:
				return '?';
		}
	}

	return c;
}




