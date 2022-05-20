
#include <stdlib.h>
#include <stdio.h>

#include "cpp.h"
#include "sti/string_int.h"


#define MAX(a,b) ((a) > (b) ? (a) : (b))



struct string_internment_table* str_table;


enum {
	_NONE = 0,
	_HASH,
	_HASH_DEF,
	_HASH_DEF_SP,
	_HASH_DEF_SP_ID,
	_HASH_DEF_SP_ID_LP,
	_MACRO_ARGS,
	_MACRO_ARGS_RP,
	_MACRO_BODY,
	
	_FOUND_NAME,
	_INV_ARGS,
};





void preprocess_file(char* path) {
	lexer_source_t* src = calloc(1, sizeof(*src));
	
	src->text = readWholeFile(path, &src->len);
	src->head = src->text;
	if(!src->text) {
		fprintf(stderr, "Failed to read file '%s'\n", path);
		free(src);
		return;
	}
	
	
	// lex the file
	string_internment_table_init(&str_table);
	
	char* _hash = strint_(str_table, "#");
	char* _define = strint_(str_table, "define");
	char* _lparen = strint_(str_table, "(");
	char* _rparen = strint_(str_table, ")");
	char* _comma = strint_(str_table, ",");
	char* _elipsis = strint_(str_table, "...");
	char* _space = strint_(str_table, " ");
	
	VEC(lexer_token_t) tokens;
	VEC_INIT(&tokens);
	
	lexer_token_t tok = {0};
	tok.start_line = 1;
	tok.alloc = 256;
	tok.text = malloc(tok.alloc * sizeof(*tok.text));
	
	cpp_macro_table_t* mt = calloc(1, sizeof(*mt));
	HT_init(&mt->names, 128);
	
	cpp_macro_name_t* mn;
	cpp_macro_def_t* m;
	
	cpp_macro_invocation_t* inv;
	cpp_token_list_t* in_arg;
	char* cached_arg = 0;
	int was_nl = 0;
	int pdepth = 0; // parenthesis nesting depth
	int state = _NONE;
	
	for(int i = 0; i < 10800; i++) { // DEBUG: sanity limits
		is_token(src, &tok);
		if(tok.len == 0) break;
		
		VEC_PUSH(&tokens, tok);
		lexer_token_t* n = &VEC_TAIL(&tokens);
		n->text = strnint_(str_table, n->text, n->len);
		
//		printf("%d:%d token: '%.*s' [%ld]\n", tok.start_line, tok.start_col, (int)tok.len, tok.text, tok.len);
		tok.start_line = tok.end_line;
		tok.start_col = tok.end_col + 1;
		

		switch(state) {
			case _NONE:
				if(was_nl && n->text == _hash) {
					state = _HASH;
					break;
				}
				
				if(!HT_get(&mt->names, n->text, &mn)) {
					printf("found macro name: %s with %d definitions\n", n->text, (int)VEC_LEN(&mn->defs));
					m = VEC_TAIL(&mn->defs);
					state = _FOUND_NAME;
				}
				
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
				
				if(HT_get(&mt->names, n->text, &mn)) {
					mn = calloc(1, sizeof(*mn));
					HT_set(&mt->names, n->text, mn);
				}
				
				VEC_PUSH(&mn->defs, m);
				
				state = _HASH_DEF_SP_ID;
				break;
			
			case _HASH_DEF_SP_ID:
				if(n->type == LEXER_TOK_SPACE) break;
				if(n->text == _lparen) {
					// function-like macro
//					printf("fn-like macro\n");
					state = _MACRO_ARGS;//_HASH_DEF_SP_ID_LP;
				}
				else {
					state = _MACRO_BODY;
				}
				break;
			
			case _HASH_DEF_SP_ID_LP:
//				printf("SP_ID_LP\n");
				state = _MACRO_ARGS;
				break;
				
			case _MACRO_ARGS:
				if(n->text == _rparen && pdepth == 0) {
					if(cached_arg) {
//						printf("pushing arg: %s\n", cached_arg);
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
//					printf("macro finished\n");
				}
				else {
//					if(n->type == LEXER_TOK_SPACE) {
//						VEC_PUSH(&m->body, _space);
//					}
//					else {
						VEC_PUSH(&m->body, n);
//					}
				}
				break;
				
				
			case _FOUND_NAME:
				if(VEC_LEN(&m->args) == 0) {
					// object-type macro
					VEC_EACH(&m->body, i, t) {
						printf("macro token: %s\n", t->text);
					}
				}
				else if(n->text == _lparen) {
					pdepth = 0;
					inv = calloc(1, sizeof(*inv));
					in_arg = calloc(1, sizeof(*in_arg));
					
					state = _INV_ARGS;
				}
				else if(n->type != LEXER_TOK_SPACE) {
					state = _NONE;
				}
				break;
				
			case _INV_ARGS:
				// collect up all the arguments being passed in
				if(n->text == _lparen) pdepth++;
				else if(n->text == _rparen) {
					if(pdepth == 0) {
						VEC_PUSH(&inv->in_args, in_arg);
						
						printf("found %ld arguments\n", VEC_LEN(&inv->in_args));
						// args done, feed out the tokens
						
						VEC_EACH(&m->body, i, bt) {
							int found = -1;
							VEC_EACH(&m->args, j, a) {
								if(a == bt->text) {
									found = j; 
									break; 
								}
							}
							
							if(found == -1) {
								printf("%s ", bt->text);
							}
							else {
								cpp_token_list_t* arg = VEC_ITEM(&inv->in_args, found);
								VEC_EACH(&arg->tokens, j, at) {
									printf("%s ", at->text);
								}
							}
							
						}
						
						printf("\n");
						
						// clean up all the invocations and arg lists
						state = _NONE;
						break;
					}
					
					pdepth = MAX(0, pdepth - 1);
				}
				else if(n->text == _comma && pdepth == 0) {
					// push the arg
					VEC_PUSH(&inv->in_args, in_arg);
					in_arg = calloc(1, sizeof(*in_arg));
				}
				else {
					VEC_PUSH(&in_arg->tokens, n);
//					printf("  arg: %s\n", n->text);
				}
				
			
				
				
				
				break;
				
		}
		
		
		
		was_nl = tok.has_newline;
	}// for
	
	
	
	
	



}










