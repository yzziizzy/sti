#ifndef __sti_parser_internal_h__
#define __sti_parser_internal_h__



#include "parser.h"


#define C_PARSER_STRING_CACHE_LIST \
	X(_comma, ",") \
	X(_star, "*") \
	X(_semi, ";") \
	X(_dquote, "\"") \
	X(_elipsis, "...") \
	X(_else, "else") \
	X(_enum, "enum") \
	X(_eq, "=") \
	X(_eqeq, "==") \
	X(_gt, ">") \
	X(_hash, "#") \
	X(_if, "if") \
	X(_lbrace, "{") \
	X(_rbrace, "}") \
	X(_lbracket, "[") \
	X(_rbracket, "]") \
	X(_lparen, "(") \
	X(_lt, "<") \
	X(_rparen, ")") \
	X(_struct, "struct") \
	X(_union, "union") \
	PARSER_TYPESPEC_LIST(Y) \
	BUILTIN_TYPE_LIST(Y) \


#define STACK_ITEM_TYPE_LIST \
	X(TYPE_SPEC) \



#define PARSER_STATE_LIST \
	X(TYPEDEF) \
	X(TYPEDEC) \
	X(TYPEDEC_ARRAY_DIM) \
	X(STRUCT) \
	X(STRUCT_IDENT) \
	X(STRUCT_IDENT_VAR) \
	X(STRUCT_DEF) \
	X(UNION) \
	X(UNION_IDENT) \
	X(UNION_IDENT_VAR) \
	X(UNION_DEF) \


#define BUILTIN_TYPE_LIST(X) \
	X(void) \
	X(char) \
	X(int) \
	X(float) \
	X(double) \
	X(_Bool) \
	X(wchar_t) \
	X(uint8_t) \
	X(uint16_t) \
	X(uint32_t) \
	X(uint64_t) \
	X(int8_t) \
	X(int16_t) \
	X(int32_t) \
	X(int64_t) \


// TODO simd intrinsics?


#define PARSER_TYPESPEC_LIST(X) \
	X(short) \
	X(long) \
	X(unsigned) \
	X(signed) \
	X(volatile) \
	X(const) \
	X(static) \
	X(extern) \
	X(register) \
	X(auto) \
	X(typedef) \

/*
#define PARSER_TYPESPEC_LIST(X) \
	X(void) \
	X(char) \
	X(short) \
	X(int) \
	X(long) \
	X(float) \
	X(double) \
	X(_Bool) \
*/

#define Y(a) X(_##a, #a)



enum {
	#define X(a, ...) TYPESPEC_ORD_##a,
		PARSER_TYPESPEC_LIST(X)
	#undef X
};

enum {
	#define X(a, ...) TYPESPEC_##a = 1 << TYPESPEC_ORD_##a,
		PARSER_TYPESPEC_LIST(X)
	#undef X	
};



typedef struct {
	char type; // s, e, u, [b]uiltin
	lexer_token_t* name; // NULL for anonymous structs
	union {
		ast_struct_def_t* stdef;
		// union
		// enum
	};
} ast_typename_t;


typedef struct stack_state {
	int id;
	
	char has_typedef;
	int stars;
	
	VEC(lexer_token_t*) type_specs;
	VEC(ast_typename_t) type_names;
	VEC(lexer_token_t*) idents;
	
	VEC(unsigned long) array_dims;
	
} stack_state_t;


typedef struct cp_ctx {
	cpp_tu_t* cpp;
	struct string_internment_table* str_table;
	cpp_token_list_t* tokens;
	
	VEC(stack_state_t) stack;
	
	
	VEC(stack_state_t) state_stack;
	stack_state_t* state;
		
	ast_tu_t* tu;

	#define X(a, b, ...) char* a;
		C_PARSER_STRING_CACHE_LIST
	#undef X
	
} cp_ctx_t;




void ast_tu_init(cp_ctx_t* ctx, ast_tu_t* tu);
ast_symbol_t* lookup_typename(ast_tu_t* tu, lexer_token_t* t);


void cp_ctx_destroy_state(stack_state_t* st);
ast_type_t* cp_ctx_process_type(cp_ctx_t* ctx);


int eat_typespecs(cp_ctx_t* ctx, long* tn);



#endif // __sti_parser_internal_h__
