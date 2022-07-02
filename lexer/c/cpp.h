#ifndef __c_cpp_h__
#define __c_cpp_h__


#include "lexer.h"

#include "sti/sti.h"



typedef struct cpp_token_list {
	VEC(lexer_token_t*) tokens;
} cpp_token_list_t;


typedef struct cpp_macro_def {
	char* name;
	unsigned int obj_like : 1;
	unsigned int fn_like : 1;
	unsigned int special : 1;
	unsigned int variadic : 1;
	VEC(char*) args;
	cpp_token_list_t body;
} cpp_macro_def_t;


typedef struct cpp_macro_invocation {
	cpp_macro_def_t* def;
	VEC(cpp_token_list_t*) in_args;
	
	VEC(cpp_token_list_t*) in_args_expanded;
	
	cpp_token_list_t* replaced; // after arg exansion and replacement, before re-scan
	cpp_token_list_t* output; // final fully-expanded token list
} cpp_macro_invocation_t;


typedef struct cpp_macro_name {
	VEC(cpp_macro_def_t*) defs;
	int invoked;
} cpp_macro_name_t;


typedef struct cpp_context {
	HT(cpp_macro_name_t*) macros;
	
	cpp_token_list_t* tokens;
	cpp_token_list_t* out;
	int cur_index;
	
	struct cpp_context* parent;
} cpp_context_t;



cpp_token_list_t* lex_file(char* path);
void preprocess_file(char* path);
void preprocess_token_list(cpp_token_list_t* tokens);

void expand_fnlike_macro(cpp_context_t* ctx, cpp_macro_invocation_t* inv);


void expand_token(cpp_context_t* ctx, cpp_token_list_t* out, cpp_token_list_t* in, size_t* cursor);


#endif
