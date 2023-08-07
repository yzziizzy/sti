#include <stdio.h>

#include "parser_internal.h"

enum {
	_NONE = 0,
	#define X(a, ...) _##a,
		PARSER_STATE_LIST
	#undef X
};




static char* state_names[] = {
	[_NONE] = "NONE",
	#define X(a, ...) [_##a] = #a,
		PARSER_STATE_LIST
	#undef X
};





#define push_state(st) push_state_(ctx, _##st)
static void push_state_(cp_ctx_t* ctx, int newID) {
	VEC_INC(&ctx->state_stack);
	stack_state_t* s = &VEC_TAIL(&ctx->state_stack);
	
	memset(s, 0, sizeof(*s));
	
	ctx->state = s;
	s->id = newID;
//	s->base = newID;
}


#define pop_state(...) pop_state_(ctx __VA_OPT__(,) __VA_ARGS__)
static void pop_state_(cp_ctx_t* ctx) {
	VEC_FREE(&ctx->state->type_specs);
	VEC_FREE(&ctx->state->type_names);
	VEC_FREE(&ctx->state->idents);
	
	VEC_LEN(&ctx->state_stack)--;
	ctx->state = &VEC_TAIL(&ctx->state_stack);
}


int is_typespec_(cp_ctx_t* ctx, lexer_token_t* t);
#define is_typespec(...) is_typespec_(ctx, __VA_ARGS__)


void cp_ctx_init(cp_ctx_t* ctx) {
	
#define X(a, b, ...) ctx->a = strint_(ctx->str_table, b);
	C_PARSER_STRING_CACHE_LIST
#undef X

}


void ast_symbol_table_init(cp_ctx_t* ctx, ast_symbol_table_t* st) {
	HT_init(&st->seu, 64);
	HT_init(&st->fbt, 64);
	
	st->next = NULL;
}

ast_symbol_t* ast_symbol_table_add_fbt_symbol(ast_symbol_table_t* st, char* name) {
	ast_symbol_t* sym = calloc(1, sizeof(*sym));
	sym->name = name;
	
	// TODO: check for redef
	HT_set(&st->fbt, name, sym);
	
	return sym;
}


void ast_symbol_table_add_builtins(cp_ctx_t* ctx, ast_symbol_table_t* st) {
	
	#define X(a, ...) ast_symbol_table_add_fbt_symbol(st, ctx->_##a);
		BUILTIN_TYPE_LIST(X)
	#undef X
}


void ast_tu_init(cp_ctx_t* ctx, ast_tu_t* tu) {
//	HT_init(&tu->types, 1000);
	
	// fill the symbol table with builtin types
	
	tu->globals = calloc(1, sizeof(*tu->globals));
	ast_symbol_table_init(ctx, tu->globals);
	
	ast_symbol_table_add_builtins(ctx, tu->globals);
}


#define dexit(...) \
do {\
	printf("%s:%d \n", __FILE__, __LINE__); \
	exit(1); \
} while(0);



static lexer_token_t* peek_token(cp_ctx_t* ctx, long* tn) {
	for(; *tn < VEC_LEN(&ctx->tokens->tokens); (*tn)++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, *tn);
	
		if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
			continue;
		}
		
		return n;
	}
	
	return NULL;
}


int parser_probe(cp_ctx_t* ctx, long* tn) {
	// with one annoying and stupid exception, we can determine what sort of code is next based on a few simple heuristics
	
	int ident_cnt = 0;
	int paren_depth = 0;
	int typename_cnt = 0;
	int star_cnt = 0;
	int typespec_cnt = 0;
	
	for(long i = *tn; i < VEC_LEN(&ctx->tokens->tokens); i++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, i);
	
		if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
			continue;
		}
		
		if(n->text == ctx->_typedef) return 't';
		
		// these could be var dec, function proto, or function def
		if(n->text == ctx->_long) { typespec_cnt++; continue; }
		if(n->text == ctx->_short) { typespec_cnt++; continue; }
		if(n->text == ctx->_unsigned) { typespec_cnt++; continue; }
		if(n->text == ctx->_signed) { typespec_cnt++; continue; }	
		if(n->text == ctx->_struct) { typespec_cnt++; continue; }
		if(n->text == ctx->_union) { typespec_cnt++; continue; }
		if(n->text == ctx->_enum) { typespec_cnt++; continue; }
		
		#define X(a, ...) if(n->text == ctx->_##a) { typename_cnt++;  continue; }
			BUILTIN_TYPE_LIST
		#undef X
		
		
		if(n->text == ctx->_star) { star_cnt++; continue; }
		
		// these are definitive signs of a variable declaration or function dec/def
		if(n->text == ctx->_inline) return 'f';
		if(n->text == ctx->_register) return 'v';
		if(n->text == ctx->_const) return 'v';
		if(n->text == ctx->_extern) return 'v';
		if(n->text == ctx->_volatile) return 'v';
		if(n->text == ctx->_static) return 'v';
		if(n->text == ctx->_auto) return 'v';
		

		
		if(n->text == ctx->_eq) return 'i'; // initialization or assignment
		
		// TODO: other assignment operators
		
		if(n->text == ctx->_lparen) return 'f'; // function call or declaration, or fucking void-cast
		if(n->text == ctx->_comma) return 'f'; // multiple declarations

		// brackets
	}
		

	
	
}


ast_var_def_t* parse_declaration(cp_ctx_t* ctx, long* tn) {
	
	int num_typespecs = eat_typespecs(ctx, tn);
	printf("got %d typespecs\n", num_typespecs);
	
	ast_type_t* type = cp_ctx_process_type(ctx);
	
	
	for(; *tn < VEC_LEN(&ctx->tokens->tokens); (*tn)++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, *tn);
		
		// could be a wild semicolon
		// could be a typedef
		// could be a bare var declaration
		if(n->text == ctx->_semi) {
			// TODO: check state for cached tokens
			ast_var_def_t* vd = calloc(1, sizeof(*vd));
			vd->type = type;
			
			if(VEC_LEN(&ctx->state->idents) == 0) {
				printf("missing identifier in variable declaration.");
			}
			else if(VEC_LEN(&ctx->state->idents) > 1) {
				printf("too many identifiers in variable declaration.");
			}
			else {
				ast_var_def_t* vd = calloc(1, sizeof(*vd));
				vd->type = type;
				vd->name = VEC_ITEM(&ctx->state->idents, 0);
				
				printf("got var def %s\n", vd->name->text);
			}
			
			cp_ctx_destroy_state(ctx->state);
			break;
		}
		
		// could be an initialization
		if(n->text == ctx->_eq) {
//			cp_ctx_process_type(ctx);
			cp_ctx_destroy_state(ctx->state);
			// TODO parse initialization
			break;
		}
		

		// could be a function
		if(n->text == ctx->_lparen) { // no K&R bullshit here. get fucked.
			cp_ctx_process_type(ctx);
			
			// TODO parse initialization
			break;
		}
		
		
		if(n->type == LEXER_TOK_IDENT) {
			VEC_PUSH(&ctx->state->idents, n);
			
			// TODO: check for commas
			
			
			
			// check function declaration
			
		}
		
			
	}
	

	return NULL;
}


ast_struct_def_t* parse_structdef(cp_ctx_t* ctx, long* tn) {
	ast_struct_def_t* stdef = calloc(1, sizeof(*stdef));
	
	for(; *tn < VEC_LEN(&ctx->tokens->tokens); (*tn)++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, *tn);
	
		if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
			continue;
		}
	
		if(n->text == ctx->_rbrace) {
			printf("finished struct definition\n");
			break;
		}
	
		VEC_PUSH(&stdef->members, parse_declaration(ctx, tn));
	}
	
	return stdef;
}



int eat_typespecs(cp_ctx_t* ctx, long* tn) {
	int found = 0;
	ast_symbol_t* sym;
	
	
	for(; *tn < VEC_LEN(&ctx->tokens->tokens); (*tn)++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, *tn);
	
		if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
			continue;
		}
		
		if(n->text == ctx->_star) {
			ctx->state->stars++;
			continue;
		}
		
		// could be a struct, union, or enum 
		if(n->text == ctx->_struct) {
			printf("got struct\n");
		
			long tn2 = *tn + 1;
			lexer_token_t* n1 = peek_token(ctx, &tn2);
			if(!n1) { printf("unexpected EOF\n"); break; }
			
			lexer_token_t* name = NULL;
			if(n1->type == LEXER_TOK_IDENT) {
				// TODO validate name
				name = n1;
				tn2++;
				n1 = peek_token(ctx, &tn2);
				printf("got struct name %s\n", name->text);
			}
			
			// a struct keyword must be followed by an identifier, an lbrace, or both
			if(n1->text == ctx->_lbrace) { // inline struct definition
				// parse struct definition
				printf("got struct brace\n");
				
				push_state(NONE);
				
				ast_struct_def_t* stdef = parse_structdef(ctx, &tn2);
				VEC_PUSH(&ctx->state->type_names, ((ast_typename_t){.type = 's', .name = name, .stdef = stdef}));
				
				pop_state();
				
				*tn = tn2;
			}
			else {
				if(name) {
					printf("struct keyword with only name\n");
					// just the name
				}
				else {
					printf("dangling struct keyword\n");
				}
			}
					
			continue;
		}
		
		if(n->text == ctx->_union) {
//			parse_uniondef(ctx, tn);
			continue;
		}
		
		if(n->text == ctx->_enum) {
//			parse_enumdef(ctx, tn);
			continue;
		}
		
		
		if(n->type == LEXER_TOK_IDENT) {
			if(is_typespec(n)) {
				if(n->text == ctx->_typedef) ctx->state->has_typedef = 1;
				
				printf("typespec\n");
				VEC_PUSH(&ctx->state->type_specs, n);
			}
			else if(sym = lookup_typename(ctx->tu, n)) {
				printf("typename\n");
				VEC_PUSH(&ctx->state->type_names, ((ast_typename_t){.type = 'b', .name = n, .stdef = NULL}));
			}
			else {
//				printf("ident\n");
//				VEC_PUSH(&ctx->state->idents, n);
				(*tn)--;
				break;
			}
		}
	
		found++;
	}
	

	return found;
}


int parse_statement(cp_ctx_t* ctx, long* tn) {
	
		
	for(; *tn < VEC_LEN(&ctx->tokens->tokens); (*tn)++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, *tn);
	
	
	
	}
	
}


int pasrse_tu_root(cp_ctx_t* ctx, long* tn) {
	ast_symbol_t* sym;
	
	
	for(; *tn < VEC_LEN(&ctx->tokens->tokens); (*tn)++) {
		lexer_token_t* n = VEC_ITEM(&ctx->tokens->tokens, *tn);
		
		// could be a declaration
		
		ast_var_def_t* vd = parse_declaration(ctx, tn);
		if(vd) {
			
			continue;
		}
		
		
		// could be a function definition
		
		if(n->text == ctx->_semi) {
			// TODO: check state for cached tokens
			cp_ctx_process_type(ctx);
			cp_ctx_destroy_state(ctx->state);
			break;
		}
		
		// could be an initialization
		if(n->text == ctx->_eq) {
//			cp_ctx_process_typedec(ctx);
			cp_ctx_destroy_state(ctx->state);
			// TODO parse initialization9
			break;
		}
		

		// could be a function pointer declaration
		if(n->text == ctx->_lparen) { 
//			cp_ctx_process_typedec(ctx);
			
			
			break;
		}
		
		
		// function or variable declaration
		if(n->type == LEXER_TOK_IDENT) {
			VEC_PUSH(&ctx->state->idents, n);
			
			// TODO: check for commas
			
			
			
			// check function declaration
			// no K&R bullshit here. get fucked.
			
		}
			
	}
	
	return 0;
}




void c_parser_tu(cpp_tu_t* cpp_tu, ast_tu_t* tu) {
	memset(tu, 0, sizeof(*tu));
	tu->cpp = cpp_tu;
	
	#define in_state(x) (_##x == ctx->state->id)

	cp_ctx_t* ctx = calloc(1, sizeof(*ctx));
	ctx->str_table = tu->cpp->str_table;
	ctx->tu = tu;
	cp_ctx_init(ctx);
	ctx->tokens = tu->cpp->root_ctx->out;
	
	ast_tu_init(ctx, tu);
	
	
	
	ast_symbol_t* sym;

	#define X(a, b, ...) char* a = ctx->a;
		C_PARSER_STRING_CACHE_LIST
	#undef X
	
	push_state(NONE);
	
	
	long tn = 0;
	while(tn < VEC_LEN(&ctx->tokens->tokens)) {
		pasrse_tu_root(ctx, &tn);
	}
	
	
	/*
	int redo = 0; // for debugging
	VEC_EACH(&tu->cpp->root_ctx->out->tokens, ni, n) {
	RESTART_REAL:
	
		sym = 1; // just in case; segfault if used
		
		if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
			continue;
		}
		
		printf("%c%ld : %s : parser loop [%s]     %s:%d:%d\n", 
			redo ? ' ' : '>',
			VEC_LEN(&ctx->state_stack), state_names[ctx->state->id],
			n->type == LEXER_TOK_SPACE ? " " : n->text,
			n->file->name, n->start_line, n->start_col
			);
		
		redo = 0;
	}
	*/
	
	
	return;

	
}











ast_symbol_t* lookup_typename(ast_tu_t* tu, lexer_token_t* t) {
	ast_symbol_t* sym = NULL;
//	
//	HT_EACH(&tu->globals->fbt, name, ast_symbol_t*, s) {
//		printf("  type: %s\n", name);
//	}
//	
	
	HT_get(&tu->globals->fbt, t->text, &sym);
	
	return sym;
}


int is_typespec_(cp_ctx_t* ctx, lexer_token_t* t) {
	
	#define X(a, ...) if(ctx->_##a == t->text) return 1;
		PARSER_TYPESPEC_LIST(X)
	#undef X

	return 0;
}












