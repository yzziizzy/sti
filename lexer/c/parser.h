


#include "cpp.h"





struct cp_ctx;
typedef struct cp_ctx cp_ctx_t;


typedef struct ast_type {
	char type; // s, e, u, [b]uiltin
	int stars;
	uint32_t specs;
	
	lexer_token_t* type_name;
} ast_type_t;


typedef struct ast_typedef {
	lexer_token_t* name;
	VEC(lexer_token_t*) names;
	VEC(lexer_token_t*) specs;
} ast_typedef_t;


typedef struct ast_var_def {
	lexer_token_t* name;
	ast_type_t* type;
	
	
	VEC(unsigned long) array_dims;

} ast_var_def_t;


typedef struct ast_struct_def {
	lexer_token_t* name; // NULL for anonymous structs
	
	VEC(ast_var_def_t*) members;
	
} ast_struct_def_t;


typedef struct ast_union_def {
	lexer_token_t* name;
	
	VEC(ast_var_def_t*) members;
	
} ast_union_def_t;



typedef struct ast_symbol {
	char type; // s, u, v, ?
	char* name;
	
	union {
		ast_struct_def_t* Struct;
		ast_union_def_t* Union;
		ast_var_def_t* Var;
	};
	
} ast_symbol_t;


typedef struct ast_symbol_table {
	// TODO: 
	
	HT(ast_symbol_t*) seu; // struct, enum, union
	HT(ast_symbol_t*) fbt; // functions, builtins, and typedefs
	
	struct ast_symbol_table* next;
} ast_symbol_table_t;



typedef struct ast_tu {
	cpp_tu_t* cpp;
	
//	VEC(ast_typedef_t*) typedefs;
	
	
//	VEC(ast_type_t*) types;
//	VEC(ast_struct_t*) structs;
//	VEC(ast_enum_t*) enums;
	
	ast_symbol_table_t* globals;

} ast_tu_t;



void cp_ctx_init(cp_ctx_t* ctx);
void c_parser_tu(cpp_tu_t* cpp_tu, ast_tu_t* tu);


