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
			BUILTIN_TYPE_LIST(X)
		#undef X
		
		
		if(n->text == ctx->_star) { star_cnt++; continue; }
		
		// these are definitive signs of a variable declaration or function dec/def
//		if(n->text == ctx->_inline) return 'f';
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
		

	
	return 1;
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
	return 1;	
}


int parse_tu_root(cp_ctx_t* ctx, long* tn) {
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

typedef struct node {
	int type;
	lexer_token_t* token;
	struct node* l, *r;
	struct node* next; 
} node;

node* cp_tu(cp_ctx_t* ctx, long* cursor);

void print_node_tree(node* n, int space);


void c_parser_tu(cpp_tu_t* cpp_tu, ast_tu_t* tu) {
	memset(tu, 0, sizeof(*tu));
	tu->cpp = cpp_tu;
	
	#define in_state(x) (_##x == ctx->state->id)

	cp_ctx_t* ctx = calloc(1, sizeof(*ctx));
	ctx->str_table = tu->cpp->str_table;
	ctx->tu = tu;
	cp_ctx_init(ctx);
	ctx->tokens = tu->cpp->root_ctx->out;
	
	//ast_tu_init(ctx, tu);
	
	
	
	//ast_symbol_t* sym;

	#define X(a, b, ...) char* a = ctx->a;
		C_PARSER_STRING_CACHE_LIST
	#undef X
	
	//push_state(NONE);
	
	
	//long tn = 0;
	//while(tn < VEC_LEN(&ctx->tokens->tokens)) {
	//	pasrse_tu_root(ctx, &tn);
	//}
	
	VEC_EACH(&ctx->tokens->tokens, ti, t) {
		printf("%ld: %s\n", ti, t->text);
	}
	
	long cursor = 0;
	node* n = cp_tu(ctx, &cursor);
	print_node_tree(n, 0);
	
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




#define AST_NODE_TYPE_LIST(X) \
	X(error) \
	X(type_spec) \
	X(struct_spec) \
	X(union_spec) \
	X(decln) \
	X(decln_spec) \
	X(decln_specs) \
	X(ident) \
	X(struct_decln_list) \
	X(struct_decln) \
	X(init_declr_list) \
	X(init_declr) \
	X(declr) \
	X(initr) \
	X(ptr) \
	X(direct_declr) \
	X(type_qual_list) \
	X(type_qual) \
	\
	X(bitfield) \
	X(logic_exp) \
	X(bit_exp) \
	X(math_exp) \
	X(equality_exp) \
	X(relational_exp) \
	X(array_decln) \
	X(incdec_prefix) \
	X(sizeof) \
	X(addressof) \
	X(deref) \
	X(unary_sign) \
	X(unary_bit_inverse) \
	X(unary_logic_not) \
	X(cast_exp) \
	X(pointer) \
	X(abs_declr) \
	X(array_index) \
	X(function_parens) \




enum {
	#define X(a,...) N_##a,
		AST_NODE_TYPE_LIST(X)
	#undef X
};

char* N__names[] = {
	#define X(a,...) [N_##a] = #a,
		AST_NODE_TYPE_LIST(X)
	#undef X
};




//typedef struct ast_declarator {
//	ast_node_t* ident;
//} ast_decl_t;
//
//typedef struct ast_declaration {
//	ast_node_t* node;
//	ast_decl_spec_t* specs;
//	ast_init_decl_t* declarators;
//} ast_decl_t;
//
//typedef struct ast_decl_spec {
//	ast_node_t* node;
//	ast_decl_spec_t* next;
//} ast_decl_spec_t;
//
//typedef struct ast_type_spec {
//	lexer_token_t* token;
//} ast_type_spec_t;
//


//typedef union ast_node {
//	int type;
//	
//	#define X(a, ...) ast_##a##_t a;
//		AST_NODE_TYPE_LIST(X)
//	#undef X
//} ast_node_t;



void print_node_tree(node* n, int space) {
	if(!n) return;
	
	for(int i = 0; i < space; i++) printf("  ");
	
	printf("%s", N__names[n->type]);
	if(n->token) {
		printf(": '%s'", n->token->text);
	}
	printf("\n");
	
	print_node_tree(n->l, space+1);
	print_node_tree(n->r, space+1);
	print_node_tree(n->next, space);
}




#define TT_inc(c) ({ lexer_token_t* t = TT_(ctx, c); (*(c))++; t; })
#define TT(c) TT_(ctx, c);
static lexer_token_t* TT_(cp_ctx_t* ctx, long* cursor) {
	lexer_token_t* t;
	
	while(1) {
		if(*cursor >= VEC_LEN(&ctx->tokens->tokens)) return &ctx->EOF_token;
		
		t = VEC_ITEM(&ctx->tokens->tokens, *cursor);
		
		if(!(t->type == LEXER_TOK_SPACE || t->type == LEXER_TOK_COMMENT)) {
			break;
		}
		
		(*cursor)++;
	}
	
	return t;
}


static lexer_token_t* TT_Eq(cp_ctx_t* ctx, long* cursor, char* text) {
	if(*cursor >= VEC_LEN(&ctx->tokens->tokens)) return NULL;
	return VEC_ITEM(&ctx->tokens->tokens, *cursor)->text == text ? VEC_ITEM(&ctx->tokens->tokens, *cursor) : NULL;
}


#define new_node(...) new_node_N(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define new_node_N(n, ...) new_node_N2(n, __VA_ARGS__)
#define new_node_N2(n, ...) new_node_##n(__VA_ARGS__)
#define new_node_1(a) new_node_(a,NULL, NULL, NULL)
#define new_node_2(a,b) new_node_(a,b, NULL, NULL)
#define new_node_3(a,b,c) new_node_(a,b,c, NULL)
#define new_node_4(a,b,c,d) new_node_(a,b,c,d)


static node* new_node_(int type, lexer_token_t* tok, node* left, node* right) {
	node* n = calloc(1, sizeof(*n));
	n->type = type;
	n->token = tok;
	n->l = left;
	n->r = right;
	return n;
}


int cp_semi(cp_ctx_t* ctx, long* cursor) {
	lexer_token_t* t = TT(cursor);
	if(t->text == ctx->_semi) {
		(*cursor)++;
		return 0;
	}
	
	return 1;
}

lexer_token_t* cp_token(cp_ctx_t* ctx, long* cursor, char* text) {
	lexer_token_t* t = TT(cursor);
	if(!t || !t->text) return NULL;
	
	if(!strcmp(t->text, text)) {
		(*cursor)++;
		return t;
	}
	
	return NULL;
}

node* cp_struct_spec(cp_ctx_t* ctx, long* cursor);
node* cp_ident(cp_ctx_t* ctx, long* cursor);
node* cp_init_declr(cp_ctx_t* ctx, long* cursor);
node* cp_decln(cp_ctx_t* ctx, long* cursor);
node* cp_struct_decln(cp_ctx_t* ctx, long* cursor);
node* cp_spec_qual_list(cp_ctx_t* ctx, long* cursor);
node* cp_declr(cp_ctx_t* ctx, long* cursor);
node* cp_struct_declr(cp_ctx_t* ctx, long* cursor);
node* cp_const_expr(cp_ctx_t* ctx, long* cursor);
node* cp_cast_expr(cp_ctx_t* ctx, long* cursor);
node* cp_direct_abstract_declr(cp_ctx_t* ctx, long* cursor);



node* cp_type_spec(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);

	int res = 0;
	lexer_token_t* t = TT(cursor);
	
	if(!t->text) return NULL;
	#define X(a,...) else if(ctx->_##a == t->text) res = 1;
		BUILTIN_TYPE_LIST(X)
		PARSER_TYPESPEC_LIST(X)
	#undef X
	
	if(res) {
		node* n = new_node(N_type_spec, t, NULL);
		(*cursor)++;
		return n;
	}
	
	node* n = cp_struct_spec(ctx, cursor);
	if(n) return n;
	
	// TODO: check type names
	// TODO: union, enum
	
	return NULL;
}



//node* cp_decln(cp_ctx_t* ctx, long* cursor) {
//	
//	long c = *cursor;
//	
//	node* n = cp_type_spec(ctx, &c);
//	
//	// TODO: type qualifier, storage class
//	
//	if(!n) return NULL;
//	
//}
//


node* cp_struct_declr_list(cp_ctx_t* ctx, long* cursor) {
	
	node* n = cp_struct_declr(ctx, cursor);
	node* tail = n;
	while(n) {
		
		long c = *cursor;
		lexer_token_t* comma = TT_inc(&c);
		if(comma->text[0] != ',') break;
		*cursor = c;
	
		node* n2 = cp_struct_declr(ctx, cursor);
		if(!n2) {
			printf("missing struct_declarator in struct_declarator_list\n");
			break;
		}
		
		tail->next = n2;
		tail = n2;
	}

	return n;
}


// struct_declr also includes bitfields
node* cp_struct_declr(cp_ctx_t* ctx, long* cursor) {

//struct_declarator
//	: declarator
//	| ':' constant_expression
//	| declarator ':' constant_expression
//	;

	node* n = cp_declr(ctx, cursor);
	if(!n) {
		lexer_token_t* t = TT(cursor);
		if(t->text[0] == ':') {
			// try const expression
		}
		
		return NULL;	
	}
	
	
	lexer_token_t* t = TT(cursor);
	if(t->text[0] == ':') {
		// try const expression
		
		
		
	}
	
	
	
	return n;	
}


node* cp_struct_decln_list(cp_ctx_t* ctx, long* cursor) {

	node* n = cp_struct_decln(ctx, cursor);
	node* tail = n;
	while(n) {
		node* n2 = cp_struct_decln(ctx, cursor);
		if(!n2) break;
		
		tail->next = n2;
		tail = n2;
	}

	return n;
}


node* cp_struct_decln(cp_ctx_t* ctx, long* cursor) {

	long c = *cursor;
	node* n1 = cp_spec_qual_list(ctx, &c);
	node* n2 = cp_struct_declr_list(ctx, &c);
	
	if(!n1 || !n2) return NULL;
	
	node* n3 = new_node(N_struct_decln, NULL, n1);
	n3->r = n2;
	
	
	lexer_token_t* semi = TT_inc(&c);
	if(semi->text[0] != ';') {
		printf("missing semicolon in struct\n");
	}
	
	*cursor = c;
	
	return n3;	
}

node* cp_type_qual(cp_ctx_t* ctx, long* cursor) {
	lexer_token_t* t = TT(cursor);
	if(strcmp("const", t->text) && strcmp("volatile", t->text)) return NULL;
	
	(*cursor)++;
	return new_node(N_type_qual, t, NULL);	
}


node* cp_spec_qual_list(cp_ctx_t* ctx, long* cursor) {
	
	node* n;
	long c = *cursor;
	
	n = cp_type_spec(ctx, &c);
	if(!n) {
		n = cp_type_qual(ctx, &c);
	}
	
	if(n) {
		n->next = cp_spec_qual_list(ctx, &c);
	}
	
	if(n) *cursor = c;
	
	return n;
}



node* cp_struct_spec(cp_ctx_t* ctx, long* cursor) {
	
	lexer_token_t* tst, *tid, *topbr, *tclbr;
	node* nid, *ndecls;
	
	tst = TT(cursor); 
	if(tst->text == ctx->_struct) {
		long c = *cursor + 1;
		
		nid = cp_ident(ctx, &c);
		
		topbr = TT_inc(&c); // todo: check
		if(topbr->text[0] != '{') {
			printf("missing opening brace on struct");
		}
		
		ndecls = cp_struct_decln_list(ctx, &c);
		
		tclbr = TT_inc(&c);
		if(tclbr->text[0] != '}') {
			printf("missing closing brace on struct");
		}
		
		node* n = new_node(N_struct_spec, tst, nid);
		n->r = ndecls;
		
		// TODO: opening/closing brace
		*cursor = c;
		return n;
	}
	
	return NULL;
}


node* cp_decln_specs(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);
	
	// TODO: storage class specs
	
	node* n = cp_type_spec(ctx, cursor);
	if(n) {
		n->next = cp_decln_specs(ctx, cursor);
		return n;
	}
	
//	node* n = cp_type_qual(ctx, cursor);
//	if(n) {
//		n->next = cp_decln_specs(ctx, cursor);
//		return n;
//	}
	
	return n;
}




node* cp_decln(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);
	
	long c = *cursor;
	
	node* n1 = cp_decln_specs(ctx, &c);
	node* n2 = cp_init_declr(ctx, &c);
	printf("%p\n", n2);
	// TODO: init_declr_list
	
	if(!n2) { // bare declarations, for structs
		if(!cp_semi(ctx, &c)) {
			*cursor = c;
			return n1;
		}
		
		printf("missing semicolon after declaration\n");
		
		return NULL;
	}
	
	cp_semi(ctx, &c);
	
	node* n = new_node(N_decln, NULL, n1);
	n->r = n2;
	
	*cursor = c;
	
	return n;
}





node* cp_ident(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);
	
	lexer_token_t* t = TT(cursor);
	
	
	if(t->type != LEXER_TOK_IDENT) return NULL;
	printf("('%s')\n", t->text);
	
	node* n = new_node(N_ident, NULL, NULL);	
	n->token = t;
	(*cursor)++;
	
	return n;
}



node* cp_declr(cp_ctx_t* ctx, long* cursor) {
	node* n = cp_ident(ctx, cursor);
	
//declarator
//	: pointer direct_declarator
//	| direct_declarator
//	;
//
//direct_declarator
//	: IDENTIFIER
//	| '(' declarator ')'
//	| direct_declarator '[' constant_expression ']'

	printf("foo2:\n");
	if(cp_token(ctx, cursor, "[")) {
		printf("foo\n");
		node* n2 = new_node(N_array_decln, NULL, n);
		
		n2->r = cp_const_expr(ctx, cursor);
	
		cp_token(ctx, cursor, "]");
		
		return n2;
	}

//	| direct_declarator '[' ']'
//	| direct_declarator '(' parameter_type_list ')'
//	| direct_declarator '(' identifier_list ')'
//	| direct_declarator '(' ')'
//	;
//
//pointer
//	: '*'
//	| '*' type_qualifier_list
//	| '*' pointer
//	| '*' type_qualifier_list pointer
//	;
	return n;
}



node* cp_init_declr(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);
	
	// TODO: should be cp_declr
//	
//init_declarator
//	: declarator
//	| declarator '=' initializer
//	;
	
	node* n = cp_declr(ctx, cursor);
	// TODO: init_declr_list
	return n;
}

node* cp_ext_decln(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);
	return cp_decln(ctx, cursor);
}

node* cp_tu(cp_ctx_t* ctx, long* cursor) {
	printf("-%s-\n", __func__);
	
	node* n = cp_ext_decln(ctx, cursor);
	node* tail = n;
	while(n) {
		node* n2 = cp_ext_decln(ctx, cursor);
		if(!n2) break;
		
		tail->next = n2;
		tail = n2;
	}
	
	return n;
}


node* cp_postfix_expr(cp_ctx_t* ctx, long* cursor) {
	return cp_ident(ctx, cursor);
}

node* cp_type_name(cp_ctx_t* ctx, long* cursor) {
	return cp_ident(ctx, cursor);
/*
type_name
	: specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator
	;

*/
}


node* cp_type_qual_list(cp_ctx_t* ctx, long* cursor) {

	return NULL;
}

	
node* cp_pointer(cp_ctx_t* ctx, long* cursor) {	
	long c = *cursor;
	lexer_token_t* t = cp_token(ctx, &c, "*");
	if(!t) return NULL;
	
	node* n2 = cp_type_qual_list(ctx, &c);
	node* n3 = cp_pointer(ctx, &c);
	
	*cursor = c;
	return new_node(N_pointer, t, n2, n3);
}



node* cp_abstract_declr(cp_ctx_t* ctx, long* cursor) {
	node* n2 = cp_pointer(ctx, cursor);
	node* n3 = cp_direct_abstract_declr(ctx, cursor);
	
	return new_node(N_abs_declr, NULL, n2, n3);
}


//
//parameter_type_list
//	: parameter_list
//	| parameter_list ',' ELLIPSIS
//	;
//
//parameter_list
//	: parameter_declaration
//	| parameter_list ',' parameter_declaration
//	;
//
//parameter_declaration
//	: declaration_specifiers declarator
//	| declaration_specifiers abstract_declarator
//	| declaration_specifiers
//	;

node* cp_direct_abstract_declr(cp_ctx_t* ctx, long* cursor) {
//	
//direct_abstract_declarator
//	| '(' ')'
//	| '(' parameter_type_list ')'
//	: '(' abstract_declarator ')'
//	| '[' ']'
//	| '[' constant_expression ']'
//	| direct_abstract_declarator '[' ']'
//	| direct_abstract_declarator '[' constant_expression ']'
//	| direct_abstract_declarator '(' ')'
//	| direct_abstract_declarator '(' parameter_type_list ')'
//	;
	
	lexer_token_t* t1 = TT(cursor);
	long c = *cursor + 1;
	lexer_token_t* t2 = TT(&c);
	
	if(!strcmp(t1->text, "(") && !strcmp(t2->text, ")")) {
		return new_node(N_function_parens);
	}
	
	if(!strcmp(t1->text, "[") && !strcmp(t2->text, "]")) {
		return new_node(N_array_index);
	}
	
	
	
	return NULL;
}


node* cp_unary_expr(cp_ctx_t* ctx, long* cursor) {
	
	long c = *cursor;
	lexer_token_t* t = TT(&c);
	
	if(!strcmp(t->text, "++") || !strcmp(t->text, "--")) {
		c++;
		node* n2 = cp_unary_expr(ctx, &c);
		
		*cursor = c;
		return new_node(N_incdec_prefix, t, n2);
	}
	
	if(!strcmp(t->text, "sizeof")) {
		c++;
//	| SIZEOF unary_expression
//	| SIZEOF '(' type_name ')'
		node* n2 = cp_type_name(ctx, &c); // TODO: error checking
		
		*cursor = c;
		return new_node(N_sizeof, t, n2);
	}
	
	if(!strcmp(t->text, "&")) {
		c++;
		node* n2 = cp_cast_expr(ctx, &c); // TODO: error checking
		
		*cursor = c;
		return new_node(N_addressof, t, n2);
	}
	
	if(!strcmp(t->text, "*")) {
		c++;
		node* n2 = cp_cast_expr(ctx, &c); // TODO: error checking
		
		*cursor = c;
		return new_node(N_deref, t, n2);
	}
	
	if(!strcmp(t->text, "-") || !strcmp(t->text, "+")) {
		c++;
		node* n2 = cp_cast_expr(ctx, &c); // TODO: error checking
		
		*cursor = c;
		return new_node(N_unary_sign, t, n2);
	}
	
	if(!strcmp(t->text, "~")) {
		c++;
		node* n2 = cp_cast_expr(ctx, &c); // TODO: error checking
		
		*cursor = c;
		return new_node(N_unary_bit_inverse, t, n2);
	}
	
	if(!strcmp(t->text, "!")) {
		c++;
		node* n2 = cp_cast_expr(ctx, &c); // TODO: error checking
		
		*cursor = c;
		return new_node(N_unary_logic_not, t, n2);
	}
	
	
	return cp_postfix_expr(ctx, cursor);	
}



node* cp_cast_expr(cp_ctx_t* ctx, long* cursor) {
	
	long c = *cursor;
	
	lexer_token_t* t = cp_token(ctx, &c, "(");
	if(t) {
		// check next token for a typename
		node* type = cp_type_name(ctx, &c);
		
		lexer_token_t* t3 = cp_token(ctx, &c, ")");
		
		node* n2 = cp_cast_expr(ctx, &c);
		if(!n2) {
			// some error
			return NULL;
		}
		
		*cursor = c;
		return new_node(N_cast_exp, NULL, type, n2);
	}
	
	return cp_unary_expr(ctx, cursor);
}



node* cp_mul_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_cast_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "*");
	if(!t) {
		t = cp_token(ctx, cursor, "/");
		if(!t) {
			t = cp_token(ctx, cursor, "%");
			if(!t) return l;
		}
	}
	
	node* r = cp_mul_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_math_exp, t, l, r);
}


node* cp_add_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_mul_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "+");
	if(!t) {
		t = cp_token(ctx, cursor, "-");
		if(!t) return l;
	}
	
	node* r = cp_add_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_math_exp, t, l, r);
}


node* cp_shift_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_add_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "<<");
	if(!t) {
		t = cp_token(ctx, cursor, ">>");
		if(!t) return l;
	}
	
	node* r = cp_shift_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_math_exp, t, l, r);
}



node* cp_relational_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_shift_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, ">");
	if(!t) {
		t = cp_token(ctx, cursor, "<");
		if(!t) {
			t = cp_token(ctx, cursor, ">=");
			if(!t) {
				t = cp_token(ctx, cursor, "<=");
				if(!t) return l;
			}
		}
	}
	
	node* r = cp_relational_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_relational_exp, t, l, r);
}


node* cp_equality_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_relational_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "==");
	if(!t) {
		t = cp_token(ctx, cursor, "!=");
		if(!t) return l;
	}
	
	node* r = cp_equality_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_equality_exp, t, l, r);
}


node* cp_bit_and_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_equality_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "&");
	if(!t) return l;
	
	node* r = cp_bit_and_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_bit_exp, t, l, r);
}

node* cp_bit_xor_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_bit_and_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "^");
	if(!t) return l;
	
	node* r = cp_bit_xor_expr(ctx, cursor);
	// TODO: error
	
	return new_node(N_bit_exp, t, l, r);
}

node* cp_bit_or_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_bit_xor_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "|");
	if(!t) return l;
	
	node* r = cp_bit_or_expr(ctx, cursor);
	// TODO: error
	
	return  new_node(N_bit_exp, t, l, r);
}


node* cp_logic_and_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_bit_or_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "&&");
	if(!t) return l;
	
	node* r = cp_logic_and_expr(ctx, cursor);
	// TODO: error
	
	node* n = new_node(N_logic_exp, t, l);
	n->r = r;
	
	return n;
}


node* cp_logic_or_expr(cp_ctx_t* ctx, long* cursor) {
	node* l = cp_logic_and_expr(ctx, cursor);
	
	lexer_token_t* t = cp_token(ctx, cursor, "||");
	if(!t) return l;
	
	node* r = cp_logic_or_expr(ctx, cursor);
	// TODO: error
	
	node* n = new_node(N_logic_exp, t, l);
	n->r = r;
	
	return n;
}


node* cp_cond_expr(cp_ctx_t* ctx, long* cursor) {
	node* n = cp_logic_or_expr(ctx, cursor);
	
	// TODO: ? : 
	
	return n;
}

node* cp_const_expr(cp_ctx_t* ctx, long* cursor) {
	return cp_cond_expr(ctx, cursor);
}






#if 0

declaration_specifiers
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers
	| type_specifier
	| type_specifier declaration_specifiers
	| type_qualifier
	| type_qualifier declaration_specifiers
	;

declaration
	: declaration_specifiers ';'
	| declaration_specifiers init_declarator_list ';'
	;

init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator
//	| declarator '=' initializer
//	;
	
declarator
	: pointer direct_declarator
	| direct_declarator
	;
	
pointer
	: '*'
	| '*' type_qualifier_list
	| '*' pointer
	| '*' type_qualifier_list pointer
	;
	
type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;

type_qualifier
	: CONST
	| VOLATILE
	;


direct_declarator
	: IDENTIFIER
//	| '(' declarator ')'
//	| direct_declarator '[' constant_expression ']'
//	| direct_declarator '[' ']'
//	| direct_declarator '(' parameter_type_list ')'
//	| direct_declarator '(' identifier_list ')'
//	| direct_declarator '(' ')'
//	;



// structs and unions:

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}'
	| struct_or_union '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER
	;

struct_or_union
	: STRUCT
	| UNION
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';'
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list
	| type_specifier
	| type_qualifier specifier_qualifier_list
	| type_qualifier
	;



struct_declarator_list
	: struct_declarator
	| struct_declarator_list ',' struct_declarator
	;

struct_declarator
	: declarator
	| ':' constant_expression
	| declarator ':' constant_expression
	;

#endif


#define C_AST_NODE_TYPE_LIST(X) \
	X(type_spec) \
	X(decln_specs) \
	X(decln) \
	X(init_declr_list) \
	X(init_declr) \
	X(declr) \
	X(initr) \
	X(ptr) \
	X(direct_declr) \
	X(type_qual_list) \
	X(type_qual) \
	X() \



typedef node* (*rec_fn)(cp_ctx_t* ctx, long* cursor);



rec_fn recognizers[] = {
//	[N_type_spec] = rec_type_spec, 
};



node* rec_sequence(cp_ctx_t* ctx, long* cursor, int* seq) {
	long c = *cursor;
	
	int* s = seq;
	while(*s) {
		
//		int ret = try_rec(ctx, &c, *s);
//		if(!ret) {
			// match failure, bail
			
//			return ret;
//		}
		// else it matches, carry on
		
		// TODO: cache the nodes somewhere
		
		s++;
	}

	*cursor = c;
	return 0;
}








