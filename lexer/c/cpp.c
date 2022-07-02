
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
	
	_FOUND_NAME = 30, 
	_INV_ARGS, // 31
};



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



void preprocess_file(char* path) {

	string_internment_table_init(&str_table);
	
	char* _hash = strint_(str_table, "#");
	char* _define = strint_(str_table, "define");
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	char* _comma = strint_(str_table, ",");
	char* _elipsis = strint_(str_table, "...");
	char* _space = strint_(str_table, " ");
	char* _va_args = strint_(str_table, "__VA_ARGS__");
	char* _va_opt = strint_(str_table, "__VA_OPTS__");

	cpp_token_list_t* tokens = lex_file(path);
	
	preprocess_token_list(tokens);
}


void preprocess_token_list(cpp_token_list_t* tokens) {
	printf("proc token list\n");
		
	char* _hash = strint_(str_table, "#");
	char* _define = strint_(str_table, "define");
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
	int pdepth = 0; // parenthesis nesting depth
	int state = _NONE;
	
	int sanity = 0;
	
	VEC_EACH(&tokens->tokens, ni, n) {
		printf(" {%ld} p token list loop [%s] %d\n", ni, n->type == LEXER_TOK_SPACE ? " " : n->text, state);
		if(sanity++ > 100) break;
		
		switch(state) {
			case _NONE:
				if(was_nl && n->text == _hash) {
					state = _HASH;
					break;
				}
				
				
				expand_token(ctx, ctx->out, tokens, &ni);
				
				break;
			
			case _HASH:
				if(n->text == _define) state = _HASH_DEF;
				else if(n->type != LEXER_TOK_SPACE) state = _NONE;
				break;
				
			case _HASH_DEF:
				if(n->type == LEXER_TOK_SPACE) {
					// start macro here
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
				m->name = n->text;
				
				if(HT_get(&ctx->macros, n->text, &mn)) {
					mn = calloc(1, sizeof(*mn));
					HT_set(&ctx->macros, n->text, mn);
				}
				
				VEC_PUSH(&mn->defs, m);
				
				state = _HASH_DEF_SP_ID;
				break;
			
			case _HASH_DEF_SP_ID:
				if(n->type == LEXER_TOK_SPACE) break;
				if(n->text == _lparen) {
					// function-like macro
//					printf("fn-like macro\n");
					m->fn_like = 1;
					state = _MACRO_ARGS;//_HASH_DEF_SP_ID_LP;
				}
				else {
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
						VEC_PUSH(&m->args, cached_arg);
						cached_arg = 0;
					}
					state = _MACRO_ARGS_RP;
				}
				else if(n->type == LEXER_TOK_IDENT) {
					cached_arg = n->text;
				}
				else if(n->text == _comma) {
					if(cached_arg) {
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
					printf("  [%s] pushing body token: '%s'\n", m->name, n->type == LEXER_TOK_SPACE ? " " : n->text);
					VEC_PUSH(&m->body.tokens, n);
				}
				break;
		
		}
		
		
		
		was_nl = n->has_newline;
	}// for
	
	
	printf("\noutput:\n");
	VEC_EACH(&ctx->out->tokens, i, t) {
		if(t->type == LEXER_TOK_SPACE) printf(" ");
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
				}
				else {
					VEC_PUSH(&in_arg->tokens, n);
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






void expand_fnlike_macro(cpp_context_t* ctx, cpp_macro_invocation_t* inv) {
	char* _va_args = strint_(str_table, "__VA_ARGS__");
	char* _va_opt = strint_(str_table, "__VA_OPT__");
	char* _va_cnt = strint_(str_table, "__VA_CNT__");
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	
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
					else if(bt->text == _va_cnt) {
						inject_number(ctx, inv->replaced, vararg_count);
					}
					else {
						VEC_PUSH(&inv->replaced->tokens, bt);
					}
				}
			}
		}
		else if(bt->text == _va_cnt) {
			inject_number(ctx, inv->replaced, vararg_count);
			goto ARG_REPLACED;
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









