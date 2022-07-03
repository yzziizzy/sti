
#include <stdlib.h>
#include <stdio.h>

#include "cpp.h"
#include "sti/string_int.h"


#define MAX(a,b) ((a) > (b) ? (a) : (b))



struct string_internment_table* str_table;


enum {
	_NONE = 0,
	_HASH, // 1
	_HASH_DEF, // 2
	_HASH_DEF_SP, // 3
	_HASH_DEF_SP_ID, // 4
	_HASH_DEF_SP_ID_LP, // 5
	_MACRO_ARGS, // 6
	_MACRO_ARGS_ELIPSIS, // 7
	_MACRO_ARGS_RP, // 8
	_MACRO_BODY, // 9
	
	_HASH_IFDEF, // 10
	_HASH_IFDEF_ID, // 11
	_HASH_IFNDEF, // 12
	_HASH_IFNDEF_ID, // 13
	_HASH_IF, // 15
	_HASH_ELSE, // 16
	_HASH_ELSEIF, // 17
	_HASH_ENDIF, // 18
	
	_FOUND_NAME = 30, 
	_INV_ARGS, // 31
	
	_SKIP_REST = 999,
};



static void inject_space(cpp_context_t* ctx, cpp_token_list_t* list) {
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_SPACE;
	t->text = strint_(str_table, " ");
	VEC_PUSH(&list->tokens, t);
}

static void inject_comma(cpp_context_t* ctx, cpp_token_list_t* list) {
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_PUNCT;
	t->text = strint_(str_table, ",");
	VEC_PUSH(&list->tokens, t);
}

static void inject_number(cpp_context_t* ctx, cpp_token_list_t* list, long num) {
	char buf[64];
	
	snprintf(buf, 64, "%ld", num);
	
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_PUNCT;
	t->text = strint_(str_table, buf);
	VEC_PUSH(&list->tokens, t);
}

static void inject_stringified(cpp_context_t* ctx, cpp_token_list_t* list, cpp_token_list_t* input) {
	char* buf;
	size_t sz = 0;
	
	VEC_EACH(&input->tokens, ti, t) {
		if(t->type == LEXER_TOK_SPACE) {
			sz++;
			continue;
		}
		
		for(char* s = t->text; *s; s++) {
			if(*s == '\\' || *s == '"') sz++;
			sz++;
		}
	}
	
	buf = malloc(sz + 3); // two quotes and a null;
	buf[0] = '"';
	buf[sz + 1] = '"';
	buf[sz + 2] = 0;
	
	char* c = buf + 1;
	VEC_EACH(&input->tokens, ti, t) {
		if(t->type == LEXER_TOK_SPACE) {
			*c++ = ' ';
			continue;
		}
		
		for(char* s = t->text; *s; s++) {
			if(*s == '\\' || *s == '"') *c++ = '\\';
			*c++ = *s;
		}
	}
	
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_STRING;
	t->text = strint_(str_table, buf);
	VEC_PUSH(&list->tokens, t);
	
	free(buf);
}


static void inject_pasted(cpp_context_t* ctx, cpp_token_list_t* list, lexer_token_t* l, lexer_token_t* r) {
	char* buf;
	
	buf = malloc(sizeof(*buf) * (1 + strlen(l->text) + strlen(r->text)));
	
	
	strcpy(buf, l->text);
	strcat(buf, r->text);
	
	lexer_source_t lx;
	lx.text = buf;
	lx.head = buf;
	lx.len = strlen(buf);
	
	lexer_token_t tok = {0};
	tok.alloc = 1 + strlen(buf);
	tok.text = malloc(tok.alloc);
	
	if(is_token(&lx, &tok) && strlen(tok.text) == lx.len) {
		lexer_token_t* t = calloc(1, sizeof(*t));
		t->type = tok.type; // TODO: probe
		t->text = strint_(str_table, buf);
		VEC_PUSH(&list->tokens, t);
		
		printf("    > pasted token: '%s'\n", buf);
	}
	else {
		VEC_PUSH(&list->tokens, l);
		inject_space(ctx, list);
		VEC_PUSH(&list->tokens, r);

		printf("    > paste failed for '%s'\n", buf);
	}
	

	free(buf);
	free(tok.text);
}

int is_defined(cpp_context_t* ctx, lexer_token_t* name) {
	return NULL != get_macro_def(ctx, name);
}

int is_integer(lexer_token_t* t, long* val) {
	char* end;
	long l = strtol(t->text, &end, 0);
	if(end != t->text + strlen(t->text)) {
		return 0; // not a valid integer
	}
	
	if(val) *val = l;
	return 1;
}


cpp_token_list_t* lex_file(char* path) {
	lexer_source_t* src = calloc(1, sizeof(*src));
	
	src->text = readWholeFile(path, &src->len);
	src->head = src->text;
	if(!src->text) {
		fprintf(stderr, "Failed to read file '%s'\n", path);
		free(src);
		return NULL;
	}
	
	
	// lex the file

	
	cpp_token_list_t* tokens = calloc(1, sizeof(*tokens));
	
	lexer_token_t tok = {0};
	tok.start_line = 1;
	tok.alloc = 256;
	tok.text = malloc(tok.alloc * sizeof(*tok.text));
	
	for(int i = 0; i < 10800; i++) { // DEBUG: sanity limits
		is_token(src, &tok);
		if(tok.len == 0) break;
		
		lexer_token_t* n = malloc(sizeof(*n));
		VEC_PUSH(&tokens->tokens, n);		
		*n = tok; 
		n->text = strnint_(str_table, n->text, n->len);
		
		tok.start_line = tok.end_line;
		tok.start_col = tok.end_col + 1;
	}
	
	return tokens;
}

// name, arity, precedence, associativity (L = 0, R = 1)
#define OPERATOR_LIST \
	X(PLUS, 2, 40, 0) \
	X(MINUS, 2, 40, 0) \
	X(MUL, 2, 30, 0) \
	X(DIV, 2, 30, 0) \
	X(MOD, 2, 30, 0) \
	X(BIT_AND, 2, 80, 0) \
	X(BIT_OR, 2, 82, 0) \
	X(BIT_NOT, 1, 20, 1) \
	X(BIT_XOR, 2, 81, 0) \
	X(SHR, 2, 50, 0) \
	X(SHL, 2, 50, 0) \
	X(LOGIC_AND, 2, 90, 0) \
	X(LOGIC_OR, 2, 91, 0) \
	X(LOGIC_NOT, 1, 20, 1) \
	X(GT, 2, 60, 0) \
	X(LT, 2, 60, 0) \
	X(GTE, 2, 60, 0) \
	X(LTE, 2, 60, 0) \
	X(NEQ, 2, 70, 0) \
	X(EQ, 2, 70, 0) \
	X(LPAREN, -1, 127, 0) \
	X(RPAREN, -1, -1, 0) \


#define X(a, ...) OP_##a,
enum {
	OP_NONE = 0,
	OPERATOR_LIST
};
#undef X

#define X(a, ...) [OP_##a] = #a,
char* operator_names[] = {
	[OP_NONE] = "none",
	OPERATOR_LIST
};
#undef X

#define X(a, b, c, d) [OP_##a] = {OP_##a, b, c, d},
cpp_stack_token_t operator_data[] = {
	[OP_NONE] = {-1, 0, 127, 0},
	OPERATOR_LIST
};
#undef X


int probe_operator_type(lexer_token_t* t) {
	char* s = t->text;
	int type = OP_NONE;
	
	switch(s[0]) {
		case '=':
			switch(s[1]) {
				case '=': type = OP_EQ; break;
			}
			break;
		case '<':
			switch(s[1]) {
				case '<': type = OP_SHL; break;
				case '=': type = OP_LTE; break;
				default: type = OP_LT; break;
			}
			break;
		case '>':
			switch(s[1]) {
				case '>': type = OP_SHR; break;
				case '=': type = OP_GTE; break;
				default: type = OP_GT; break;
			}
			break;
		case '!':
			switch(s[1]) {
				case '=': type = OP_NEQ; break;
				default: type = OP_LOGIC_NOT; break;
			}
			break;
		case '~': type = OP_BIT_NOT; break;
		case '^': type = OP_BIT_XOR; break;
		case '|':
			switch(s[1]) {
				case '|': type = OP_LOGIC_OR; break;
				default: type = OP_BIT_OR; break;
			}
			break;
		case '&':
			switch(s[1]) {
				case '&': type = OP_LOGIC_AND; break;
				default: type = OP_BIT_AND; break;
			}
			break;
		case '/': type = OP_DIV; break;
		case '*': type = OP_MUL; break;
		case '%': type = OP_MOD; break;
		case '-': type = OP_MINUS; break;
		case '+': type = OP_PLUS; break;
		case ')': type = OP_RPAREN; break;
		case '(': type = OP_LPAREN; break;
	}
	
	return type;
}


void reduce(cpp_context_t* ctx) {
	int op;
	
	VEC_POP(&ctx->oper_stack, op);
	
	long lval, rval, res = 0;
	
	if(operator_data[op].arity > 0) VEC_POP(&ctx->value_stack, rval);
	if(operator_data[op].arity > 1) VEC_POP(&ctx->value_stack, lval);

	switch(op) {
		case OP_EQ: res = lval == rval; break;
		case OP_NEQ: res = lval != rval; break;
		case OP_GTE: res = lval >= rval; break;
		case OP_LTE: res = lval <= rval; break;
		case OP_GT: res = lval > rval; break;
		case OP_LT: res = lval < rval; break;
		case OP_PLUS: res = lval + rval; break;
		case OP_MINUS: res = lval - rval; break;
		case OP_MUL: res = lval * rval; break;
		case OP_DIV: res = lval / rval; break;
		case OP_MOD: res = lval % rval; break;
		case OP_BIT_NOT: res = ~rval; break;
		case OP_LOGIC_NOT: res = !rval; break;
		case OP_LOGIC_AND: res = lval && rval; break;
		case OP_LOGIC_OR: res = lval || rval; break;
		case OP_BIT_AND: res = lval & rval; break;
		case OP_BIT_OR: res = lval | rval; break;
		case OP_BIT_XOR: res = lval ^ rval; break;
		case OP_SHR: res = lval >> rval; break;
		case OP_SHL: res = lval << rval; break;
			
		
		case OP_LPAREN:
			printf("     evaluationg lparen!!!!\n");
			return;
			
		case OP_RPAREN:
			printf("     evaluating rparen!!!!\n");			
			return;
		
	}


	VEC_PUSH(&ctx->value_stack, res);
}


void preprocess_file(char* path) {

	string_internment_table_init(&str_table);
	
	char* _hash = strint_(str_table, "#");
	char* _define = strint_(str_table, "define");
	char* _if = strint_(str_table, "if");
	char* _ifdef = strint_(str_table, "ifdef");
	char* _ifndef = strint_(str_table, "ifndef");
	char* _else = strint_(str_table, "else");
	char* _elseif = strint_(str_table, "elseif");
	char* _endif = strint_(str_table, "endif");
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	char* _comma = strint_(str_table, ",");
	char* _elipsis = strint_(str_table, "...");
	char* _space = strint_(str_table, " ");
	char* _va_args = strint_(str_table, "__VA_ARGS__");
	char* _va_opt = strint_(str_table, "__VA_OPTS__");
	char* _va_narg = strint_(str_table, "__VA_NARG__");

	cpp_token_list_t* tokens = lex_file(path);
	
	preprocess_token_list(tokens);
}


void preprocess_token_list(cpp_token_list_t* tokens) {
	printf("proc token list\n");
		
	char* _hash = strint_(str_table, "#");
	char* _define = strint_(str_table, "define");
	char* _if = strint_(str_table, "if");
	char* _ifdef = strint_(str_table, "ifdef");
	char* _ifndef = strint_(str_table, "ifndef");
	char* _else = strint_(str_table, "else");
	char* _elseif = strint_(str_table, "elseif");
	char* _endif = strint_(str_table, "endif");
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	char* _comma = strint_(str_table, ",");
	char* _elipsis = strint_(str_table, "...");
	char* _space = strint_(str_table, " ");
	
	
	cpp_context_t* ctx = calloc(1, sizeof(*ctx));
	HT_init(&ctx->macros, 128);
	ctx->tokens = tokens;
	ctx->out = calloc(1, sizeof(*ctx->out));
	
	cpp_macro_name_t* mn;
	cpp_macro_def_t* m;
	
	cpp_macro_invocation_t* inv;
	cpp_token_list_t* in_arg;
	char* cached_arg = 0;
	int was_nl = 0;
	int was_ws = 0;
	int pdepth = 0; // parenthesis nesting depth
	
	int cond_depth = 0;
	int out_enable = 1; // don't output anything for failed conditionals
	
	int state = _NONE;
	
	int sanity = 0;
	
	VEC_EACH(&tokens->tokens, ni, n) {
		printf(" %s {%ld} p token list loop [%s] %d\n", out_enable ? "" : "X", ni, n->type == LEXER_TOK_SPACE ? " " : n->text, state);
		if(sanity++ > 400) break;
		
		switch(state) {
			case _NONE:
				if(was_nl && n->text == _hash) {
					state = _HASH;
					break;
				}
				
				if(out_enable)
					expand_token(ctx, ctx->out, tokens, &ni);
				
				break;
			
			case _HASH:
				if(n->text == _define) state = _HASH_DEF;
				else if(n->text == _if) state = _HASH_IF;
				else if(n->text == _ifdef) state = _HASH_IFDEF;
				else if(n->text == _ifndef) state = _HASH_IFNDEF;
				else if(n->text == _else) state = _HASH_ELSE;
				else if(n->text == _elseif) state = _HASH_ELSEIF;
				else if(n->text == _endif) state = _HASH_ENDIF;
				else if(n->type != LEXER_TOK_SPACE) state = _NONE;
				break;
				
			case _HASH_DEF:
				if(n->type == LEXER_TOK_SPACE) {
					// start macro here
					if(out_enable)
						m = calloc(1, sizeof(*m));
					
					state = _HASH_DEF_SP;
				}
				else {
					fprintf(stderr, "Whitespace is required after #define at %d:%d\n", n->start_line, n->start_col);
					state = _NONE;
				}
				break;
			
			case _HASH_DEF_SP:
				if(n->type != LEXER_TOK_IDENT) {
					fprintf(stderr, "An Identifier is required after #define at %d:%d\n", n->start_line, n->start_col);
					state = _NONE;
				}
				
				// macro name
				if(out_enable) {
					m->name = n->text;
					
					if(HT_get(&ctx->macros, n->text, &mn)) {
						mn = calloc(1, sizeof(*mn));
						HT_set(&ctx->macros, n->text, mn);
					}
					
					VEC_PUSH(&mn->defs, m);
				}
				
				state = _HASH_DEF_SP_ID;
				break;
			
			case _HASH_DEF_SP_ID:
				if(n->type == LEXER_TOK_SPACE) break;
				if(n->text == _lparen) {
					// function-like macro
//					printf("fn-like macro\n");
					if(out_enable)
						m->fn_like = 1;
					
					state = _MACRO_ARGS;//_HASH_DEF_SP_ID_LP;
				}
				else {
					if(out_enable)
						m->obj_like = 1;
					
					state = _MACRO_BODY;
					ni--;
				}
				break;
			
			case _HASH_DEF_SP_ID_LP:
//				printf("SP_ID_LP\n");
				state = _MACRO_ARGS;
				break;
				
			case _MACRO_ARGS:
				if(n->text == _rparen && pdepth == 0) {
					if(cached_arg) {
						// printf("pushing arg: %s\n", cached_arg);
						if(out_enable)
							VEC_PUSH(&m->args, cached_arg);
						cached_arg = 0;
					}
					state = _MACRO_ARGS_RP;
				}
				else if(n->type == LEXER_TOK_IDENT) {
					if(out_enable) 
						cached_arg = n->text;
				}
				else if(n->text == _comma) {
					if(cached_arg) {
						if(out_enable) 
							VEC_PUSH(&m->args, cached_arg);
						cached_arg = 0;
					}
					else {
						fprintf(stderr, "An argument name identifier is required at %d:%d\n", n->start_line, n->start_col);
					}
				}
				else if(n->text == _elipsis) {
					// variadic macro
					if(pdepth != 0) {
						fprintf(stderr, "Varargs elipsis encountered inside nested parenthesis.\n");
					}
					
					if(out_enable)
						m->variadic = 1;
					
					state = _MACRO_ARGS_ELIPSIS;
				}
				else if(n->type != LEXER_TOK_SPACE) {
					fprintf(stderr, "Unexpected token '%s' at %d:%d\n", n->text, n->start_line, n->start_col);
				}
				break;

			case _MACRO_ARGS_ELIPSIS:
				if(n->text == _rparen) {
					state = _MACRO_ARGS_RP;
				}
				else if(n->type != LEXER_TOK_SPACE) {
					fprintf(stderr, "Unexpected token '%s' at %d:%d\n", n->text, n->start_line, n->start_col);
				}
				break;
			
			case _MACRO_ARGS_RP:
				if(n->type != LEXER_TOK_SPACE) {
					fprintf(stderr, "Whitespace required after macro parameter list at %d:%d\n", n->start_line, n->start_col);
				}
				state = _MACRO_BODY;
				break;
			
			case _MACRO_BODY:
				if(n->has_newline) {
					state = _NONE;
				}
				else {
					if(!out_enable) break;
					
					if(n->type == LEXER_TOK_SPACE) {
						if(VEC_LEN(&m->body.tokens) == 0) break;
					}
					printf("  [%s] pushing body token: '%s'\n", m->name, n->type == LEXER_TOK_SPACE ? " " : n->text);
					
					if(n->type != LEXER_TOK_SPACE) {
						if(was_ws) {
							inject_space(ctx, &m->body);
						}
						VEC_PUSH(&m->body.tokens, n);
						
						was_ws = 0;
					}
					else {
						was_ws = 1;
					}
				}
				break;
				
				
				
			/*-------------------------------
					Simple Conditionals
			---------------------------------*/
				
				
			case _HASH_IFDEF:
				if(n->type == LEXER_TOK_IDENT) {
					if(!is_defined(ctx, n)) {
						out_enable = 0;
					}
					
					cond_depth++;
					if(n->has_newline) state = _NONE;
					else state = _SKIP_REST;
				}
				
				break;
				
			case _HASH_IFNDEF:
				if(n->type == LEXER_TOK_IDENT) {
					if(is_defined(ctx, n)) {
						out_enable = 0;
					}
					
					cond_depth++;
					if(n->has_newline) state = _NONE;
					else state = _SKIP_REST;
				}
				break;
			

				
			case _HASH_ELSE:
				if(cond_depth == 0) {
					fprintf(stderr, "Found unmatched #else\n");
				}
				else if(cond_depth == 1) {
					out_enable = !out_enable;
				}
				
				if(n->has_newline) state = _NONE;
				else state = _SKIP_REST;
				break;
				
			case _HASH_ENDIF:
				if(cond_depth == 0) {
					fprintf(stderr, "Found unmatched #endif\n");
				}
				else if(cond_depth == 1) {
					cond_depth = 0;
					out_enable = 1;
				}
				else {
					cond_depth--;
				}
				
				if(n->has_newline) state = _NONE;
				else state = _SKIP_REST;
				break;
			
			
			
			/*------------------------------------
					Complicated Conditionals
			--------------------------------------*/
			
			case _HASH_IF:
			
				if(n->type == LEXER_TOK_NUMBER) {
					long val;
					if(is_integer(n, &val)) {
						VEC_PUSH(&ctx->value_stack, val);
						printf("  pushing integer %ld to the stack\n", val);
					}
				
				}
				
				if(n->type == LEXER_TOK_IDENT) {
					
				
					// expand macros
					
					// non-integers are 0
					
					// check for operators
				}
				
				if(n->type == LEXER_TOK_PUNCT) {
					// check for operators, rest are 0
					
					int op = probe_operator_type(n);
					
					if(op == OP_LPAREN) {
						VEC_PUSH(&ctx->oper_stack, op);
						printf("  pushing lparen to the stack\n");
					}
					else if(op == OP_RPAREN) {
						printf("     executing rparen\n");
						do {
							int top = VEC_TAIL(&ctx->oper_stack);
							printf("       - %s\n", operator_names[top]);
							
							if(top == OP_LPAREN) {
								printf("       - found lparen, exiting loop (top value: %ld)\n", VEC_TAIL(&ctx->value_stack));
								VEC_POP1(&ctx->oper_stack);
								break;
							}
							
							reduce(ctx);
							
						} while(VEC_LEN(&ctx->oper_stack));
					
					}
					else if(op > 0) {
						int top = 0;
						if(VEC_LEN(&ctx->oper_stack)) 
							top = VEC_TAIL(&ctx->oper_stack);
						
						printf("top: %d, op: %d\n", top, op);
						if(operator_data[top].prec < operator_data[op].prec) {
							// if(top.prec == r->prec && r->assoc == STI_OP_ASSOC_LEFT) break;
							reduce(ctx);
						}
						
						VEC_PUSH(&ctx->oper_stack, op);
						printf("  pushing operator %s to the stack\n", operator_names[op]);
					}
					else {
						VEC_PUSH(&ctx->value_stack, 0);
						printf("  pushing 0 to the stack due to '%s'\n", n->text);
					}
				}
				
				if(n->has_newline) {
					
					while(VEC_LEN(&ctx->oper_stack)) {
						reduce(ctx);
					}
					
					long final; VEC_POP(&ctx->value_stack, final);
					cond_depth++;
					out_enable = final != 0;
					
					state = _NONE;
				}
				
				break;
		
			case _SKIP_REST:
				// skip everything to the end of the line
				if(n->has_newline) {
					state = _NONE;
				}
				break;

		}
		
		
		
		was_nl = n->has_newline;
	}// for
	
	
	printf("\noutput:\n");
	VEC_EACH(&ctx->out->tokens, i, t) {
		if(t->type == LEXER_TOK_COMMENT) {}
		else if(t->type == LEXER_TOK_SPACE) printf(" ");
		else printf("%s ", t->text);
	}
	printf("\n");

}

// returns a raw invocation struct with no replacements 
cpp_macro_invocation_t* collect_invocation_args(cpp_context_t* ctx, cpp_token_list_t* input, cpp_macro_def_t* m, size_t* cursor) {
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	char* _comma = strint_(str_table, ",");
//	char* _elipsis = strint_(str_table, "...");
	char* _space = strint_(str_table, " ");
	
	cpp_macro_name_t* mn;
//	cpp_macro_def_t* m;
	
	cpp_macro_invocation_t* inv;
	
//	if(!m->fn_like) return NULL;
	
	
	cpp_token_list_t* in_arg;
	char* cached_arg = 0;
	int pdepth = 0; // parenthesis nesting depth
	int state = _FOUND_NAME;
	int i;
	int argn = 0;
	int was_ws = 0;
	
	// BUG: should read from a cursor of some kind
	for(i = *cursor; i < VEC_LEN(&input->tokens); i++) {
		lexer_token_t* n = VEC_ITEM(&input->tokens, i);

		printf("     collecting '%s' (arg# %d)\n", n->text, argn);
		switch(state) {	
			case _FOUND_NAME:
				if(n->text == _lparen) {
					inv = calloc(1, sizeof(*inv));
					inv->def = m;
					in_arg = calloc(1, sizeof(*in_arg));
				
					state = _INV_ARGS;
				}
				else if(n->type != LEXER_TOK_SPACE) {
					// missing parens
					
					printf("No parens found for macro invocation '%s', not expanding (found '%s')\n", m->name, n->text);
					return NULL;
				}
				
				 // skip witespace between the name and the opening paren 
				break;
				
			case _INV_ARGS:
				// collect up all the arguments being passed in
				if(n->text == _lparen) {
					VEC_PUSH(&in_arg->tokens, n);
					pdepth++;
				}
				else if(n->text == _rparen) {
					if(pdepth == 0) {
						// BUG? should push a space if there was whitespace?
						VEC_PUSH(&inv->in_args, in_arg);
						
						printf("found %ld arguments:\n", VEC_LEN(&inv->in_args));
						
						VEC_EACH(&inv->in_args, ai, a) {
							printf("    %ld) ", ai);
							VEC_EACH(&a->tokens, aai, aa) printf("%s ", aa->text);
							printf("\n");
						}
						printf("\n");
						// args done
						
						goto DONE;
					}
					else VEC_PUSH(&in_arg->tokens, n);
					
					pdepth = MAX(0, pdepth - 1);
				}
				else if(n->text == _comma && pdepth == 0) {
					// push the arg
					VEC_PUSH(&inv->in_args, in_arg);
					in_arg = calloc(1, sizeof(*in_arg));
					argn++;
					was_ws = 0;
				}
				else {
				
					if(n->type == LEXER_TOK_SPACE) {
						if(VEC_LEN(&in_arg->tokens) == 0) break;
					}
//					printf("  [%s] pushing body token: '%s'\n", m->name, n->type == LEXER_TOK_SPACE ? " " : n->text);
					
					if(n->type != LEXER_TOK_SPACE) {
						if(was_ws) {
							printf("     --space injected\n");
							inject_space(ctx, &m->body);
						}
						printf("     --arg pushed '%s'\n", n->text);
						VEC_PUSH(&in_arg->tokens, n);
						
						was_ws = 0;
					}
					else {
						was_ws = 1;
					}
				
					
//					printf("  arg: %s\n", n->text);
				}
				
				break;
		}
	}
	
	// no parens because the list ended
	return NULL;
	
DONE:
		
	*cursor = i;
	return inv;
}



// gets the next token without consideration of macro expansion
// used for seeing if there's an opening paren after the current token
lexer_token_t* peek_token_raw(cpp_context_t* ctx) {
//	if(!ctx) return ctx->EOF;
	if(!ctx) return NULL;

	int i = ctx->cur_index + 1;
	if(VEC_LEN(&ctx->tokens->tokens) < i) {
		peek_token_raw(ctx->parent);
	}
	
	return VEC_ITEM(&ctx->tokens->tokens, i);
}


cpp_macro_def_t* get_macro_def(cpp_context_t* ctx, lexer_token_t* query) {
	
	cpp_macro_name_t* name = NULL;
	if(HT_get(&ctx->macros, query->text, &name) || !name) {
		return NULL;
	}
	
	return VEC_TAIL(&name->defs);
}


cpp_token_list_t* expand_token_list(cpp_context_t* ctx, cpp_token_list_t* in) {
	
	cpp_token_list_t* out = calloc(1, sizeof(*out));
	
	
	VEC_EACH(&in->tokens, ti, t) {
		expand_token(ctx, out, in, &ti);
	}

	return out;
} 



void expand_token(cpp_context_t* ctx, cpp_token_list_t* out, cpp_token_list_t* in, size_t* cursor) {
	
	lexer_token_t* t = VEC_ITEM(&in->tokens, *cursor);
	
	cpp_macro_def_t* m = get_macro_def(ctx, t);
	if(!m) {
		// just a regular token. push it to the output
		printf("    regular token, pushing '%s' to output\n", t->type == LEXER_TOK_SPACE ? " " : t->text);
		VEC_PUSH(&out->tokens, t);
	}
	else if(m->fn_like) {
		printf("    fnlike, checking '%s' for parens\n", t->text);
		
		size_t c2 = *cursor + 1;
		cpp_macro_invocation_t* inv = collect_invocation_args(ctx, in, m, &c2);
		if(!inv) {
			// it's fn like but not being invoked due to lack of subsequent parens
			printf("    non-invoked fnlike, pushing '%s' to output\n", t->text);
			VEC_PUSH(&out->tokens, t);
			return;
		}
		
		printf("    fnlike, expanding '%s'\n", t->text);
		
		expand_fnlike_macro(ctx, inv);
		VEC_CAT(&out->tokens, &inv->output->tokens);
		*cursor = c2;
		
		// TODO: process fn-like macro expansion
		
	}
	else if(m->obj_like) {
		printf("    objlike, expanding '%s' to ->", t->text);
		
		VEC_EACH(&m->body.tokens, bi, b) { 
			printf("%s ", b->text); 
		} printf("<-\n");
		
		cpp_token_list_t* expanded = expand_token_list(ctx, &m->body);
		VEC_CAT(&out->tokens, &expanded->tokens);
		
		VEC_FREE(&expanded->tokens);
		free(expanded);
	}
	else if(m->special) {
		// __FILE__, etc
	}

}


ssize_t arg_index(cpp_macro_def_t* m, lexer_token_t* name) {

	VEC_EACH(&m->args, ani, aname) {
		if(aname == name->text) {
			return ani;
		}
	}
	
	return -1;
}

lexer_token_t* next_real_token(cpp_token_list_t* list, size_t* cursor) {
	
	size_t i = *cursor + 1;
	
	for(; i < VEC_LEN(&list->tokens); i++) {
		lexer_token_t* t = VEC_ITEM(&list->tokens, i);
		
		if(t->type != LEXER_TOK_SPACE && t->type != LEXER_TOK_COMMENT) {
			*cursor = i;
			return t;
		}
	}
	
	return NULL;
}


void expand_fnlike_macro(cpp_context_t* ctx, cpp_macro_invocation_t* inv) {
	char* _va_args = strint_(str_table, "__VA_ARGS__");
	char* _va_opt = strint_(str_table, "__VA_OPT__");
	char* _va_narg = strint_(str_table, "__VA_NARG__");
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	char* _hash = strint_(str_table, "#");
	char* _concat = strint_(str_table, "##");
	
	cpp_macro_def_t* m = inv->def;
	
	
	// argument prescan
	printf("  -- argument prescan --\n");
	VEC_EACH(&inv->in_args, i, arg) {
		VEC_PUSH(&inv->in_args_expanded, expand_token_list(ctx, arg));
	}
	
	int vararg_count = VEC_LEN(&inv->in_args) - VEC_LEN(&m->args); 
	
	
	printf("  -- argument replacement --\n");
	// fill replacement list
	inv->replaced = calloc(1, sizeof(*inv->replaced));
	
	VEC_EACH(&m->body.tokens, bti, bt) {
		
		// special lookahead for ##
		if(bti < VEC_LEN(&m->body.tokens) - 1) {
			size_t next_ind = bti;
			lexer_token_t* bt_next = next_real_token(&m->body, &next_ind);
			
			if(bt_next->text == _concat) {
				printf("   Token pasting operator encountered\n");
				
				if(bti >= VEC_LEN(&m->body.tokens) - 2) {
					fprintf(stderr, "Token pasting operator at end of macro body\n");
				}
				else {
					// the token after the ##
					size_t c_ind = next_ind;
					lexer_token_t* ct = next_real_token(&m->body, &c_ind);
					
					printf("       body tokens being pasted: '%s' ## '%s'\n", bt->text, ct->text);
					
					lexer_token_t* paste_l, *paste_r;
					
					ssize_t bai = arg_index(m, bt);
					if(bai > -1) {
						cpp_token_list_t* l_arg_tokens = VEC_ITEM(&inv->in_args, bai);
						// append all but the last of the left tokens (bt)
						for(int i = 0; i < VEC_LEN(&l_arg_tokens->tokens) - 1; i++) {
							VEC_PUSH(&inv->replaced->tokens, VEC_ITEM(&l_arg_tokens->tokens, i));
						}
						
						paste_l = VEC_ITEM(&l_arg_tokens->tokens, VEC_LEN(&l_arg_tokens->tokens) - 1);
					}
					else {
						// literal token
						paste_l = bt;
					}
					
					
					cpp_token_list_t* r_arg_tokens;
					ssize_t cai = arg_index(m, ct);
					if(cai > -1) {
						// handle the argument replacement
						r_arg_tokens = VEC_ITEM(&inv->in_args, cai);
						paste_r = VEC_ITEM(&r_arg_tokens->tokens, 0);
					}
					else {
						// literal token
						paste_r = ct;
					}
					
					printf("       literal tokens being pasted: '%s' ## '%s'\n", paste_l->text, paste_r->text);
					// paste the last of bt with the first of ct
					// BUG: right now the CPP will not validate if it's a valid token. It will just paste it.
					inject_pasted(ctx, inv->replaced, paste_l, paste_r);
					
					
					// append the rest of ct tokens
					if(cai > -1) {
						for(int i = 1; i < VEC_LEN(&r_arg_tokens->tokens); i++) {
							VEC_PUSH(&inv->replaced->tokens, VEC_ITEM(&r_arg_tokens->tokens, i));
						}
					}
					
					bti = c_ind + 1;
					continue;
				}
			}
		}
	
	
		if(bt->text == _va_args) {
			// special __VA_ARGS__ handling
			
			size_t start_arg = VEC_LEN(&m->args);
			
			for(int i = start_arg; i < VEC_LEN(&inv->in_args); i++) {
				if(i > start_arg) inject_comma(ctx, inv->replaced);
				VEC_CAT(&inv->replaced->tokens, &VEC_ITEM(&inv->in_args_expanded, i)->tokens);
			}
		
			goto ARG_REPLACED;
		}
		else if(bt->text == _va_opt) {
			int pdepth = 0;
			int got_lparen = 0;
			for(bti++; bti < VEC_LEN(&m->body.tokens); bti++) {
				bt = VEC_ITEM(&m->body.tokens, bti);
				
				if(!got_lparen) {
					if(bt->type == LEXER_TOK_SPACE) continue;
					if(bt->text == _lparen) {
						got_lparen = 1;
						continue;
					}
					
					fprintf(stderr, "Missing lparen after __VA_OPT__\n");
					break;
				}
				
				if(bt->text == _lparen) {
					pdepth++;
				}
				else if(bt->text == _rparen) {
					if(pdepth == 0) goto ARG_REPLACED;
					pdepth--;
				}
				
				
				if(vararg_count > 0) { // __VA_OPT__ only works if there are args left
					if(bt->text == _va_args) {
						size_t start_arg = VEC_LEN(&m->args);
						for(int i = start_arg; i < VEC_LEN(&inv->in_args); i++) {
							if(i > start_arg) inject_comma(ctx, inv->replaced);
							VEC_CAT(&inv->replaced->tokens, &VEC_ITEM(&inv->in_args_expanded, i)->tokens);
						}
					
					}
					else if(bt->text == _va_narg) {
						inject_number(ctx, inv->replaced, vararg_count);
					}
					else {
						VEC_PUSH(&inv->replaced->tokens, bt);
					}
				}
			}
		}
		else if(bt->text == _va_narg) {
			inject_number(ctx, inv->replaced, vararg_count);
			goto ARG_REPLACED;
		}
		else if(bt->text == _hash) {
		
			bti++;
			if(bti >= VEC_LEN(&m->body.tokens)) {
				fprintf(stderr, "Stringifier operator at end of macro body.\n");
				break;
			}
			
			bt = VEC_ITEM(&m->body.tokens, bti);
			
			// TODO: #__VA_ARGS__, et al
			VEC_EACH(&m->args, ani, aname) {
				if(aname == bt->text) {
					
					inject_stringified(ctx, inv->replaced, VEC_ITEM(&inv->in_args, ani));
				
					goto ARG_REPLACED;
				}
			}
			
			
			
			goto ARG_REPLACED;
		}
		
		// TODO: implement lookahead
		else if(bt->text == _concat) {
			
			
			bti++;
			if(bti >= VEC_LEN(&m->body.tokens)) {
				fprintf(stderr, "Token pasting operator at end of macro body.\n");
				break;
			}
			
		}
		
		else {
			// normal tokens
			VEC_EACH(&m->args, ani, aname) {
				if(aname == bt->text) {
					VEC_CAT(&inv->replaced->tokens, &VEC_ITEM(&inv->in_args_expanded, ani)->tokens);
					goto ARG_REPLACED;
				}
			}
		}
		
		// no arg replacement done.
		VEC_PUSH(&inv->replaced->tokens, bt);
		
	ARG_REPLACED:
	}
	
	
	// re-scan the final list
	inv->output = calloc(1, sizeof(*inv->output));
	
	printf("  -- final rescan --\n");
	inv->output = expand_token_list(ctx, inv->replaced);


	// mark macro disabled

	return;
}









