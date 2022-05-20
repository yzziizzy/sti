#ifndef __c_cpp_h__
#define __c_cpp_h__


#include "lexer.h"

#include "sti/sti.h"



typedef struct cpp_token_list {
	VEC(lexer_token_t*) tokens;
} cpp_token_list_t;

typedef struct cpp_macro_def {
	char* name;
	VEC(char*) args;
	VEC(lexer_token_t*) body;
} cpp_macro_def_t;

typedef struct cpp_macro_invocation {
	cpp_macro_def_t* def;
	VEC(cpp_token_list_t*) in_args;
	
	
} cpp_macro_invocation_t;

typedef struct cpp_macro_name {
	VEC(cpp_macro_def_t*) defs;
} cpp_macro_name_t;


typedef struct cpp_macro_table {
	HT(cpp_macro_name_t*) names;
	
} cpp_macro_table_t;


void preprocess_file(char* path);





#endif
