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


typedef struct cpp_stack_token { 
	char type;
	char arity; 
	char prec; 
	char assoc; 
} cpp_stack_token_t;

typedef struct cpp_context {
	HT(cpp_macro_name_t*) macros; // very mutable
	VEC(cpp_macro_def_t*) all_defs; // used for reference later
	
	cpp_token_list_t* tokens;
	cpp_token_list_t* out;
	int cur_index;
	
	// for expressions in if's
	VEC(int) oper_stack;
	VEC(lexer_token_t*) value_stack;
	cpp_token_list_t exp_buffer;
	
	struct cpp_context* parent;
} cpp_context_t;




#define CPP_STRING_CACHE_LIST \
	X(_comma, ",") \
	X(_concat, "##") \
	X(_define, "define") \
	X(_dquote, "\"") \
	X(_elipsis, "...") \
	X(_else, "else") \
	X(_elseif, "elseif") \
	X(_endif, "endif") \
	X(_error, "error") \
	X(_gt, ">") \
	X(_hash, "#") \
	X(_ident, "ident") \
	X(_if, "if") \
	X(_ifdef, "ifdef") \
	X(_ifndef, "ifndef") \
	X(_include, "include") \
	X(_line, "line") \
	X(_lparen, "(") \
	X(_lt, "<") \
	X(_pragma, "pragma") \
	X(_rparen, ")") \
	X(_space, " ") \
	X(_undef, "undef") \
	X(_va_args, "__VA_ARGS__") \
	X(_va_narg, "__VA_NARG__") \
	X(_va_opt, "__VA_OPTS__") \
	X(_warning, "warning")



typedef struct cpp_tu {
	cpp_context_t* root_ctx;
	
	VEC(char*) system_inc_dirs;
	VEC(char*) local_inc_dirs;
	
	struct string_internment_table* str_table;
	
#define X(a, ...) char* a;
	CPP_STRING_CACHE_LIST
#undef X
} cpp_tu_t;



cpp_token_list_t* lex_file(cpp_tu_t* tu, char* path);
void preprocess_file(cpp_tu_t* tu, cpp_context_t* ctx, char* path);
void preprocess_token_list(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* tokens);

cpp_macro_def_t* get_macro_def(cpp_context_t* ctx, lexer_token_t* query);
void expand_fnlike_macro(cpp_tu_t* tu, cpp_context_t* ctx, cpp_macro_invocation_t* inv);
cpp_token_list_t* expand_token_list(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* in);
lexer_token_t* next_real_token(cpp_token_list_t* list, size_t* cursor);

void expand_token(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* out, cpp_token_list_t* in, size_t* cursor);


#endif
