
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <libgen.h>
#include <linux/limits.h> // for PATH_MAX

#include "cpp.h"
#include "sti/string_int.h"


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


#include "cpp_exp.c"


void undef_macro(cpp_tu_t* tu, char* name);


#define CPP_STATE_LIST \
	X(HASH) \
	X(HASH_DEF) \
	X(HASH_DEF_SP) \
	X(HASH_DEF_SP_ID) \
	X(HASH_DEF_SP_ID_LP) \
	X(MACRO_ARGS) \
	X(MACRO_ARGS_ELIPSIS) \
	X(MACRO_ARGS_RP) \
	X(MACRO_BODY) \
	X(FOUND_NAME) \
	X(INV_ARGS) \
	\
	X(HASH_IFDEF) \
	X(HASH_IFDEF_ID) \
	X(HASH_IFNDEF) \
	X(HASH_IFNDEF_ID) \
	X(HASH_IF) \
	X(HASH_ELSE) \
	X(HASH_ELSEIF) \
	X(HASH_ENDIF) \
	\
	X(HASH_ERROR) \
	X(HASH_WARNING) \
	\
	X(HASH_INC) \
	X(HASH_INC_SP) \
	X(HASH_INC_SP_LT) \
	\
	X(HASH_UNDEF) \
	X(HASH_UNDEF_SP) \
	\
	X(SKIP_REST)
	
enum {
	_NONE = 0,
	#define X(a, ...) _##a,
		CPP_STATE_LIST
	#undef X
};


char* state_names[] = {
	[_NONE] = "NONE",
	#define X(a, ...) [_##a] = #a,
		CPP_STATE_LIST
	#undef X
};


static void inject_space(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* list) {
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_SPACE;
	t->text = strint_(tu->str_table, " ");
	VEC_PUSH(&list->tokens, t);
}

static void inject_comma(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* list) {
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_PUNCT;
	t->text = strint_(tu->str_table, ",");
	VEC_PUSH(&list->tokens, t);
}

static void inject_number(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* list, long num) {
	char buf[64];
	
	snprintf(buf, 64, "%ld", num);
	
	lexer_token_t* t = calloc(1, sizeof(*t));
	t->type = LEXER_TOK_PUNCT;
	t->text = strint_(tu->str_table, buf);
	VEC_PUSH(&list->tokens, t);
}

static void inject_stringified(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* list, cpp_token_list_t* input) {
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
	t->text = strint_(tu->str_table, buf);
	VEC_PUSH(&list->tokens, t);
	
	free(buf);
}


static void inject_pasted(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* list, lexer_token_t* l, lexer_token_t* r) {
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
	
	if(is_token(&lx, &tok) && tok.len == lx.len) {
		lexer_token_t* t = calloc(1, sizeof(*t));
		t->type = tok.type; // TODO: probe
		t->text = strint_(tu->str_table, buf);
		VEC_PUSH(&list->tokens, t);
		
//		printf("    > pasted token: '%s'\n", buf);
	}
	else {
		VEC_PUSH(&list->tokens, l);
		inject_space(tu, ctx, list);
		VEC_PUSH(&list->tokens, r);

//		printf("    > paste failed for '%s'\n", buf);
	}
	

	free(buf);
	free(tok.text);
}


cpp_token_list_t* lex_file(cpp_tu_t* tu, cpp_file_t* file) {
	lexer_source_t* src = calloc(1, sizeof(*src));
	
	src->text = readWholeFile(file->full_path, &src->len);
	src->head = src->text;
	if(!src->text) {
		fprintf(stderr, "Failed to read file '%s'\n", file->full_path);
		free(src);
		return NULL;
	}
	
	
	// lex the file

	
	cpp_token_list_t* tokens = calloc(1, sizeof(*tokens));
	
	lexer_token_t tok = {0};
	tok.start_line = 1;
	tok.alloc = 256;
	tok.text = malloc(tok.alloc * sizeof(*tok.text));
	
	for(int i = 0; i < 2000000000; i++) { // DEBUG: sanity limits
		is_token(src, &tok);
		if(tok.len == 0) break;
		
		lexer_token_t* n = malloc(sizeof(*n));
		VEC_PUSH(&tokens->tokens, n);		
		*n = tok;
		n->alloc = n->len; // the interred string doesn't waste any space
		n->text = strnint_(tu->str_table, n->text, n->len);
		n->file = file;
		
		tok.start_line = tok.end_line;
		tok.start_col = tok.end_col + 1;
	}
	
	return tokens;
}


void cpp_tu_init(cpp_tu_t* tu) {
	HT_init(&tu->files, 64);
	HT_init(&tu->macros, 128);
	
	string_internment_table_init(&tu->str_table);
	
#define X(a, b, ...) tu->a = strint_(tu->str_table, b);
	CPP_STRING_CACHE_LIST
#undef X

	tu->fn_buf_len = 0;
	tu->fn_buf_alloc = 512;
	tu->filename_buffer = malloc(sizeof(*tu->filename_buffer) * tu->fn_buf_alloc);
	
	tu->initialized = 1;
}

//static int is_regular_file(char* path) {
//	struct stat st;
//	if(!stat(path, &st)) {
//		if((st.st_mode & S_IFMT) == S_IFREG) return 1;
//	}
//	return 0;
//}

static cpp_file_t* init_file(cpp_tu_t* tu, char* full_path, char* include_name) {
	cpp_file_t* file;
	
	file = calloc(1, sizeof(*file));
	file->name = strdup(include_name);
	file->dir = dirname(strdup(full_path));
	file->full_path = strdup(full_path);
	
	HT_set(&tu->files, full_path, file);
	
	return file;
} 


cpp_file_t* cpp_tu_get_file(cpp_tu_t* tu, char* cwd, char* path, char is_system) {
	cpp_file_t* file;
	char* full_path, *full_path_bad;
	
	if(!HT_get(&tu->files, path, &file)) {
		return file;
	}
	
	// construct a path and search the directories
	if(is_system) {
		// check system headers first
		VEC_EACH(&tu->system_inc_dirs, i, dir) {
			full_path = path_join(dir, path);
			
			// check the cache before hitting the filesystem
			if(!HT_get(&tu->files, full_path, &file)) {
				return file;
			}
			
			if(is_regular_file(full_path)) {
				
				// found the file
//				printf("Found system header '%s'\n", full_path);
				
				file = init_file(tu, full_path, path);
				file->is_system_header = 1;
//				free(full_path);
				return file;
			}
			
			free(full_path);
		}
	
	}
	
	// check the current directory
	full_path_bad = malloc(PATH_MAX + 100);
	strcpy(full_path_bad, cwd);
	strcat(full_path_bad, "/");
	strcat(full_path_bad, path);
	
	//path_join(cwd, path);
	full_path = malloc(PATH_MAX + 100);
	realpath(full_path_bad, full_path);
	free(full_path_bad);
	
	if(!HT_get(&tu->files, full_path, &file)) {
		return file;
	}
	
	if(is_regular_file(full_path)) {
				
		// found the file
//		printf("Found local include '%s'\n", full_path);
		
		file = init_file(tu, full_path, path);
		free(full_path);
		return file;
	}
	free(full_path);
	
	
	// check local include directories next
	VEC_EACH(&tu->local_inc_dirs, i, dir) {
		full_path_bad = path_join(dir, path);
		full_path = realpath(full_path_bad, NULL);
		free(full_path_bad);
		
		// check the cache before hitting the filesystem
		if(!HT_get(&tu->files, full_path, &file)) {
			return file;
		}
		
		if(is_regular_file(full_path)) {
				
			// found the file
//			printf("Found local include '%s'\n", full_path);
			
			file = init_file(tu, full_path, path);
			free(full_path);
			return file;
		}
		
		free(full_path);	
	}
	
	
	return NULL;
}

void preprocess_file(cpp_tu_t* tu, cpp_context_t* parent, char* path, char is_system) {
	int is_first = 0;
	if(!tu->initialized) {
		cpp_tu_init(tu);
		is_first = 1;
	}
	
	char* dir;
	if(parent && parent->file) {
		dir = parent->file->dir;
	}
	else {
		dir = "./";
	}
	
	
	cpp_file_t* file = cpp_tu_get_file(tu, dir, path, is_system);
	if(!file) {
		fprintf(stderr, "Could not find file '%s'\n", path);
		return;
	}
	
//	printf("Including file: %s\n", file->full_path);
	
	// lex the file on first load
	if(!file->raw_tokens) {
		file->raw_tokens = lex_file(tu, file);
		if(!file->raw_tokens) return;
	}
	
	
	cpp_context_t* ctx = calloc(1, sizeof(*ctx));
	if(is_first) tu->root_ctx = ctx;
	

	ctx->file = file;
	ctx->out = calloc(1, sizeof(*ctx->out));
	ctx->parent = parent;
	if(parent) VEC_PUSH(&parent->children, ctx);
	ctx->tokens = file->raw_tokens;
	
	preprocess_token_list(tu, ctx, file->raw_tokens);
	
	// copy the expanded output into the including file
	if(parent) {
		VEC_CAT(&parent->out->tokens, &ctx->out->tokens);
	}
	
	return;
}


void preprocess_token_list(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* tokens) {
//	printf("proc token list\n");
		
#define X(a, b, ...) char* a = tu->a;
	CPP_STRING_CACHE_LIST
#undef X
	
	cpp_macro_name_t* mn;
	cpp_macro_def_t* m;
	
	cpp_macro_invocation_t* inv;
	cpp_token_list_t* in_arg;
	char* cached_arg = 0;
	int was_nl = 1; // the first line starts a line
	int was_ws = 1;
	int pdepth = 0; // parenthesis nesting depth
	
//	int cond_depth = 0;
	int out_enable = 1; // don't output anything for failed conditionals
//	int highest_enabled_level = 0;
//	int highest_disabled_level = 99999999;
//	VEC(cpp_macro_if_t*) cond_stack;
//	VEC_INIT(&cond_stack);
	
	cpp_macro_if_t* cond_head = NULL;
//	VEC_PUSH(&cond_stack, 1);	
	
	int state = _NONE;
	
	int sanity = 0;
	
	VEC_EACH(&tokens->tokens, ni, n) {
		/*
		printf(" %s {%ld} wnl:%d p token list loop [%s] %s     %s:%d:%d\n", 
			out_enable ? "E" : "X", 
			ni,
			was_nl, 
			n->type == LEXER_TOK_SPACE ? " " : n->text,
			state_names[state],
			ctx->file->name, n->start_line, n->start_col
			); //*/
//		if(sanity++ > 400) break;
		
		
		switch(state) {
			case _NONE:
//				printf("hash: %p\n", _hash);
				if(was_nl && n->text == _hash) {
					state = _HASH;
					break;
				}
				
				if(out_enable)
					expand_token(tu, ctx, ctx->out, tokens, &ni);
				
				break;
			
			case _HASH:
				if(n->text == _define) state = _HASH_DEF;
				else if(n->text == _undef) state = _HASH_UNDEF;
				else if(n->text == _error) state = _HASH_ERROR;
				else if(n->text == _warning) state = _HASH_WARNING;
				else if(n->text == _include) state = _HASH_INC;
				else if(n->text == _ifdef) {
					
					cpp_macro_if_t* mif = calloc(1, sizeof(*mif));
					mif->type = 'd';
					mif->decl_line = n->start_line;
					mif->raw = calloc(1, sizeof(*mif->raw));
					
					if(!cond_head) {
						VEC_PUSH(&tu->ifs, mif);
					}
					
					mif->parent = cond_head;
					cond_head = mif;
					
					if(mif->parent) {
						mif->disabled_higher = mif->parent->disabled_higher || !mif->parent->net_enabled;
					}
					
					state = _HASH_IFDEF;
				}
				else if(n->text == _ifndef) {
					
					cpp_macro_if_t* mif = calloc(1, sizeof(*mif));
					mif->type = 'n';
					mif->decl_line = n->start_line;
					mif->raw = calloc(1, sizeof(*mif->raw));
					
					if(!cond_head) {
						VEC_PUSH(&tu->ifs, mif);
					}
					
					mif->parent = cond_head;
					cond_head = mif;
					
					if(mif->parent) {
						mif->disabled_higher = mif->parent->disabled_higher || !mif->parent->net_enabled;
					}
					
					state = _HASH_IFNDEF;
				}
				else if(n->text == _else) {
					if(!cond_head) {
						fprintf(stderr, "#else without matching #if at %s:%d:%d\n", ctx->file->full_path, n->start_line, n->start_col);
						state = _SKIP_REST;
						break;
					}
					
					cpp_macro_if_t* mif = calloc(1, sizeof(*mif));
					mif->type = 'e';
					mif->decl_line = n->start_line;
					
					cond_head->next = mif;
					mif->prev = cond_head;
					mif->first = cond_head->first ? cond_head->first : cond_head;
					mif->parent = cond_head->parent;
					
					if(mif->parent) {
						mif->disabled_higher = mif->parent->disabled_higher || !mif->parent->net_enabled;
					}
					
					cond_head = mif;
					
					state = _HASH_ELSE;
				}
				else if(n->text == _elif) {
					if(!cond_head) {
						fprintf(stderr, "#elif without matching #if at %s:%d:%d\n", ctx->file->full_path, n->start_line, n->start_col);
						state = _SKIP_REST;
						break;
					}
					
					cpp_macro_if_t* mif = calloc(1, sizeof(*mif));
					mif->type = 'l';
					mif->decl_line = n->start_line;
					mif->raw = calloc(1, sizeof(*mif->raw));
					
					cond_head->next = mif;
					mif->prev = cond_head;
					mif->first = cond_head->first ? cond_head->first : cond_head;
					mif->parent = cond_head->parent;
					
					if(mif->parent) {
						mif->disabled_higher = mif->parent->disabled_higher || !mif->parent->net_enabled;
					}
					
					state = _HASH_ELSEIF;
				}
				else if(n->text == _if) {
					
					cpp_macro_if_t* mif = calloc(1, sizeof(*mif));
					mif->type = 'i';
					mif->decl_line = n->start_line;
					mif->raw = calloc(1, sizeof(*mif->raw));
					
					if(!cond_head) {
						VEC_PUSH(&tu->ifs, mif);
					}
					
					mif->parent = cond_head;
					cond_head = mif;
					
					if(mif->parent) {
						mif->disabled_higher = mif->parent->disabled_higher || !mif->parent->net_enabled;
					}
					
					state = _HASH_IF;
				}
				else if(n->text == _endif) {
					if(!cond_head) {
						fprintf(stderr, "#endif without matching #if at %d:%d\n", n->start_line, n->start_col);
						state = _SKIP_REST;
						break;
					}
					
					cond_head = cond_head->parent;
					if(cond_head) {
						out_enable = cond_head->net_enabled;
					}
					else {
						out_enable = 1; // default on
					}
					
					if(n->has_newline) state = _NONE;
					else state = _SKIP_REST;
					break;
				}
				else if(n->text == _line) state = _SKIP_REST;
				else if(n->text == _pragma) state = _SKIP_REST;
				else if(n->text == _ident) state = _SKIP_REST;
				else if(n->type != LEXER_TOK_SPACE && n->type != LEXER_TOK_COMMENT) { state = _SKIP_REST;
				
					printf("_endif: %p '%s', token: %p, '%s'\n", _endif, _endif, n->text, n->text);
					fprintf(stderr, "Unknown PP directive: %s at %s:%d:%d\n", n->text, ctx->file->full_path, n->start_line, n->start_col);
					
					exit(1);
				}
				break;
				
			case _HASH_DEF:
				if(n->type == LEXER_TOK_SPACE) {
					// start macro here
					if(out_enable)
						m = calloc(1, sizeof(*m));
					
					state = _HASH_DEF_SP;
				}
				else {
					fprintf(stderr, "Whitespace is required after #define at %s:%d:%d\n", ctx->file->full_path, n->start_line, n->start_col);
					state = _NONE;
				}
				break;
			
			case _HASH_DEF_SP:
				if(n->type != LEXER_TOK_IDENT) {
					fprintf(stderr, "An Identifier is required after #define at %s:%d:%d\n", ctx->file->full_path, n->start_line, n->start_col);
					state = _NONE;
				}
				
				// macro name
				if(out_enable) {
					m->name = n->text;
					
					if(HT_get(&tu->macros, n->text, &mn)) {
						mn = calloc(1, sizeof(*mn));
						HT_set(&tu->macros, n->text, mn);
					}
					
					VEC_PUSH(&mn->defs, m);
					VEC_PUSH(&tu->all_defs, m);
				}
				
				state = _HASH_DEF_SP_ID;
				break;
			
			case _HASH_DEF_SP_ID:
				if(n->has_newline) {
					state = _NONE;
					break;
				}
				
				// spaces are not allowed between the macro name and the fn parens
				if(n->type == LEXER_TOK_SPACE) {
					if(out_enable)
						m->obj_like = 1;
					
					state = _MACRO_BODY;
				}
				
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
				if(n->has_newline) {
					fprintf(stderr, "Unexpected linebreak at %s:%d:%d\n", ctx->file->full_path, n->start_line, n->start_col);
					state = _NONE;
					break;
				}
				if(n->text == _rparen && pdepth == 0) {
					if(cached_arg) {
						if(out_enable)
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
						if(out_enable) 
							VEC_PUSH(&m->args, cached_arg);
						cached_arg = 0;
					}
					else {
						fprintf(stderr, "An argument name identifier is required at %s:%d:%d\n", ctx->file->full_path, n->start_line, n->start_col);
						exit(1);
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
					fprintf(stderr, "Unexpected token '%s' at %s:%d:%d (%d)\n", n->text, ctx->file->full_path, n->start_line, n->start_col, __LINE__);
				}
				break;

			case _MACRO_ARGS_ELIPSIS:
				if(n->text == _rparen) {
					state = _MACRO_ARGS_RP;
				}
				else if(n->type != LEXER_TOK_SPACE) {
					fprintf(stderr, "Unexpected token '%s' at %s:%d:%d (%d)\n", n->text, ctx->file->full_path, n->start_line, n->start_col, __LINE__);
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
//					printf("  [%s] pushing body token: '%s'\n", m->name, n->type == LEXER_TOK_SPACE ? " " : n->text);
					
					if(n->type != LEXER_TOK_SPACE) {
						if(was_ws) {
							inject_space(tu, ctx, &m->body);
						}
						VEC_PUSH(&m->body.tokens, n);
						
						was_ws = 0;
					}
					else {
						was_ws = 1;
					}
				}
				break;
				
				
							
			case _HASH_UNDEF:
				if(!out_enable) {
					state = _SKIP_REST;
					break;
				}
				
				if(n->type == LEXER_TOK_SPACE) {
					state = _HASH_UNDEF_SP;
				}
				else {
					fprintf(stderr, "Whitespace is required after #undef at %d:%d\n", n->start_line, n->start_col);
					state = _NONE;
				}
				break;
			
			case _HASH_UNDEF_SP:
				if(n->type != LEXER_TOK_IDENT) {
					fprintf(stderr, "An Identifier is required after #undef at %d:%d\n", n->start_line, n->start_col);
					state = _NONE;
				}
				
				undef_macro(tu, n->text);			
				
				state = _SKIP_REST;
				break;
				
				
			/*-----------------------
					Diagnostics
			-------------------------*/
			case _HASH_ERROR:
				if(!out_enable) {
					state = _SKIP_REST;
					break;
				}
				
				if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
					printf(" ");
				}
				else {
					printf(n->text);
				}
				
				if(n->has_newline) {
					printf("\n");
					state = _NONE;
				}
				break;
				
			case _HASH_WARNING:
				if(!out_enable) {
					state = _SKIP_REST;
					break;
				}
				
				if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
					printf(" ");
				}
				else {
					printf(n->text);
				}
				
				if(n->has_newline) {
					printf("\n");
					state = _NONE;
				}
				break;
				
			/*-------------------------------
					Simple Conditionals
			---------------------------------*/
				
				
			case _HASH_IFDEF:
				if(n->type == LEXER_TOK_IDENT) {
					// todo: save tokens
//					cond_head->expanded = expand_token_list(tu, ctx, &cond_head->raw.tokens);
					cond_head->result = is_defined(tu, n);
					
					cond_head->net_enabled = (cond_head->result != 0) && !cond_head->disabled_higher;
					out_enable = cond_head->net_enabled;
					
					if(n->has_newline) state = _NONE;
					else state = _SKIP_REST;
				}
				
				break;
				
			case _HASH_IFNDEF:
				if(n->type == LEXER_TOK_IDENT) {
					cond_head->result = !is_defined(tu, n);
					
					cond_head->net_enabled = (cond_head->result != 0) && !cond_head->disabled_higher;
					out_enable = cond_head->net_enabled;
					
					if(n->has_newline) state = _NONE;
					else state = _SKIP_REST;
				}
				break;
			

	
				
			case _HASH_ELSE:
				
				cond_head->net_enabled = (!cond_head->prev->net_enabled) && !cond_head->disabled_higher;
				out_enable = cond_head->net_enabled;
				
				if(n->has_newline) state = _NONE;
				else state = _SKIP_REST;
				break;
				
			case _HASH_ENDIF:
				
				if(!cond_head) {
					fprintf(stderr, "#endif without matching #if at %d:%d\n", n->start_line, n->start_col);
					state = _SKIP_REST;
					break;
				}
				
				cond_head = cond_head->parent;
				if(cond_head) {
					out_enable = cond_head->net_enabled;
				}
				else {
					out_enable = 1; // default on
				}
				
				if(n->has_newline) state = _NONE;
				else state = _SKIP_REST;
				break;
			
			
			
			/*------------------------------------
					Complicated Conditionals
			--------------------------------------*/
			
			case _HASH_IF:

				if(n->type != LEXER_TOK_SPACE && n->type != LEXER_TOK_COMMENT) {
					VEC_PUSH(&cond_head->raw->tokens, n);
				}
				
				
				// expand macros and do the evaluation at the end of the line
				if(n->has_newline) {
					cond_head->expanded = expand_token_list(tu, ctx, cond_head->raw);
					cond_head->result = eval_exp(tu, ctx, cond_head->expanded);
					
					cond_head->net_enabled = (cond_head->result != 0) && !cond_head->disabled_higher;
					out_enable = cond_head->net_enabled;
					
					state = _NONE;
				}
				break;
				
						
			case _HASH_ELSEIF:
				// buffer up all the expression tokens first
				if(n->type != LEXER_TOK_SPACE && n->type != LEXER_TOK_COMMENT) {
					VEC_PUSH(&cond_head->raw->tokens, n);;
				}
				
				// expand macros and do the evaluation at the end of the line
				if(n->has_newline) {
					cond_head->expanded = expand_token_list(tu, ctx, cond_head->raw);
					cond_head->result = eval_exp(tu, ctx, cond_head->expanded);
					
					cond_head->net_enabled = (cond_head->result != 0) && !cond_head->disabled_higher;
					out_enable = cond_head->net_enabled;
					
					state = _NONE;
				}
				break;
				
				
			/*---------------------
					Includes
			-----------------------*/
			
			// Include syntax is somewhat complicated. It uses alternate semantics for the file names
			//   than normal C code, and the < > version can potentially have otherwise invalid lexer
			//   tokens inside it. The lexer for this preprocessor divides and classifies tokens, it 
			//   does not actually remove any of them, including comments and invalid characters.
			//   The #include handing code takes advantage of this in parsing filenames literally.
			
			case _HASH_INC: 
				if(!out_enable) {
					state = _SKIP_REST;
					break;
				}
				
				tu->fn_buf_len = 0;
				if(n->type == LEXER_TOK_SPACE) {
					state = _HASH_INC_SP;
				}
				else {
					fprintf(stderr, "Invalid token after #include: '%s'\n", n->text);
					state = _SKIP_REST;
				}
				break;
				
			case _HASH_INC_SP: 
				if(n->type == LEXER_TOK_SPACE || n->type == LEXER_TOK_COMMENT) {
					break;
				}
				else if(n->type == LEXER_TOK_STRING) { // normal file name
					// strip the double quotes out
					if(tu->fn_buf_alloc < n->len) {
						tu->fn_buf_alloc = tu->fn_buf_alloc * 2 + n->len;
						tu->filename_buffer = realloc(tu->filename_buffer, sizeof(*tu->filename_buffer) * tu->fn_buf_alloc);
					}
						
					strncpy(tu->filename_buffer, n->text + 1, n->len - 2);
					tu->fn_buf_len = n->len - 2;
					tu->filename_buffer[tu->fn_buf_len] = 0; 
					
					preprocess_file(tu, ctx, tu->filename_buffer, 0);
					
					state = _SKIP_REST;
				}
				else if(n->text == _lt) { // system header
					state = _HASH_INC_SP_LT;
				}
				else if(n->type == LEXER_TOK_IDENT) { // expand macro
					fprintf(stderr, "Indirect includes NYI\n");
					state = _SKIP_REST;
				}
				else {
					fprintf(stderr, "Invalid token after #include (2): '%s' at %s:%d:%d\n", n->text, ctx->file->full_path, n->start_line, n->start_col);
					state = _SKIP_REST;
				}
				break;
				
			case _HASH_INC_SP_LT:
				// concat all the token text before reaching another doublequote
				// escapes are ignored
				
				
				if(n->text == _gt) {
					// do the include
					preprocess_file(tu, ctx, tu->filename_buffer, 1);

					state = _SKIP_REST;
				}
				else if(n->has_newline) {
					// issue warning,
					// do the include
					fprintf(stderr, "Unexpected EOL in #include at %d:%d\n", n->start_line, n->start_col);
					state = _NONE;
				}
				else {
					// concat tokens
					if(tu->fn_buf_alloc < tu->fn_buf_len + n->len + 1) {
						tu->fn_buf_alloc = tu->fn_buf_alloc * 2 + n->len;
						tu->filename_buffer = realloc(tu->filename_buffer, sizeof(*tu->filename_buffer) * tu->fn_buf_alloc);
					}
					
					strncpy(tu->filename_buffer + tu->fn_buf_len, n->text, n->len);
					tu->fn_buf_len += n->len;
					tu->filename_buffer[tu->fn_buf_len] = 0; 
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
	
}

// returns a raw invocation struct with no replacements 
cpp_macro_invocation_t* collect_invocation_args(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* input, cpp_macro_def_t* m, size_t* cursor) {
	char* _lparen = tu->_lparen;
	char* _rparen = tu->_rparen;
	char* _comma = tu->_comma;
	char* _space = tu->_space;
	
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

//		printf("     collecting '%s' (arg# %d)\n", n->text, argn);
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
					
//					printf("No parens found for macro invocation '%s', not expanding (found '%s')\n", m->name, n->text);
					return NULL;
				}
				else {
//					printf("     skipping space\n");
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
						
						/*
						printf("found %ld arguments:\n", VEC_LEN(&inv->in_args));
						
						VEC_EACH(&inv->in_args, ai, a) {
							printf("    %ld) ", ai);
							VEC_EACH(&a->tokens, aai, aa) printf("%s ", aa->text);
							printf("\n");
						}
						printf("\n");
						// args done
						*/
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
//							printf("     --space injected\n");
							inject_space(tu, ctx, &m->body);
						}
//						printf("     --arg pushed '%s'\n", n->text);
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


cpp_macro_def_t* get_macro_def(cpp_tu_t* tu, lexer_token_t* query) {
//	if(!tu) return NULL;
	
	cpp_macro_name_t* name = NULL;
	if(HT_get(&tu->macros, query->text, &name) || !name) {
		return NULL; //return get_macro_def(tu->parent, query);
	}
	
	return VEC_TAIL(&name->defs);
}


cpp_token_list_t* expand_token_list(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* in) {
	
	cpp_token_list_t* out = calloc(1, sizeof(*out));
	
	
	VEC_EACH(&in->tokens, ti, t) {
		expand_token(tu, ctx, out, in, &ti);
	}

	return out;
} 



void expand_token(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* out, cpp_token_list_t* in, size_t* cursor) {
	
	lexer_token_t* t = VEC_ITEM(&in->tokens, *cursor);
	
	cpp_macro_def_t* m = get_macro_def(tu, t);
	if(!m) {
		// just a regular token. push it to the output
//		printf("    regular token, pushing '%s' to output\n", t->type == LEXER_TOK_SPACE ? " " : t->text);
		VEC_PUSH(&out->tokens, t);
	}
	else if(m->fn_like) {
//		printf("    fnlike, checking '%s' for parens\n", t->text);
		
		size_t c2 = *cursor + 1;
		cpp_macro_invocation_t* inv = collect_invocation_args(tu, ctx, in, m, &c2);
		if(!inv) {
			// it's fn like but not being invoked due to lack of subsequent parens
//			printf("    non-invoked fnlike, pushing '%s' to output\n", t->text);
			VEC_PUSH(&out->tokens, t);
			return;
		}
		
//		printf("    fnlike, expanding '%s'\n", t->text);
		
		expand_fnlike_macro(tu, ctx, inv);
		VEC_CAT(&out->tokens, &inv->output->tokens);
		*cursor = c2;
		
		// TODO: process fn-like macro expansion
		
	}
	else if(m->obj_like) {
		/*
		printf("    objlike, expanding '%s' to ->", t->text);
		
		VEC_EACH(&m->body.tokens, bi, b) { 
			printf("%s ", b->text); 
		} printf("<-\n");
		*/
		cpp_token_list_t* expanded = expand_token_list(tu, ctx, &m->body);
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
//	printf("li: %ld, len: %ld\n", i, VEC_LEN(&list->tokens));
	for(; i < VEC_LEN(&list->tokens); i++) {
		lexer_token_t* t = VEC_ITEM(&list->tokens, i);
		
		if(t->type != LEXER_TOK_SPACE && t->type != LEXER_TOK_COMMENT) {
			*cursor = i;
			return t;
		}
		
//		printf("i: %ld \n", i);
	}
	
	return NULL;
}


void expand_fnlike_macro(cpp_tu_t* tu, cpp_context_t* ctx, cpp_macro_invocation_t* inv) {
	char* _va_args = tu->_va_args;
	char* _va_opt = tu->_va_opt;
	char* _va_narg = tu->_va_narg;
	char* _lparen = tu->_lparen;
	char* _rparen = tu->_rparen;
	char* _hash = tu->_hash;
	char* _concat = tu->_concat;
	
	cpp_macro_def_t* m = inv->def;
	
	// argument prescan
//	printf("  -- argument prescan --\n");
	VEC_EACH(&inv->in_args, i, arg) {
		VEC_PUSH(&inv->in_args_expanded, expand_token_list(tu, ctx, arg));
	}
	
	int vararg_count = VEC_LEN(&inv->in_args) - VEC_LEN(&m->args); 
	
	
//	printf("  -- argument replacement --\n");
	// fill replacement list
	inv->replaced = calloc(1, sizeof(*inv->replaced));
	
	VEC_EACH(&m->body.tokens, bti, bt) {
		
		// special lookahead for ##
		if(bti < VEC_LEN(&m->body.tokens) - 1) {
			size_t next_ind = bti;
			lexer_token_t* bt_next = next_real_token(&m->body, &next_ind);
			
			if(bt_next && bt_next->text == _concat) {
//				printf("   Token pasting operator encountered\n");
				
				if(bti >= VEC_LEN(&m->body.tokens) - 2) {
					fprintf(stderr, "Token pasting operator at end of macro body\n");
				}
				else {
					// the token after the ##
					size_t c_ind = next_ind;
					lexer_token_t* ct = next_real_token(&m->body, &c_ind);
					
//					printf("       body tokens being pasted: '%s' ## '%s'\n", bt->text, ct->text);
					
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
					
//					printf("       literal tokens being pasted: '%s' ## '%s'\n", paste_l->text, paste_r->text);
					// paste the last of bt with the first of ct
					// BUG: right now the CPP will not validate if it's a valid token. It will just paste it.
					inject_pasted(tu, ctx, inv->replaced, paste_l, paste_r);
					
					
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
				if(i > start_arg) inject_comma(tu, ctx, inv->replaced);
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
							if(i > start_arg) inject_comma(tu, ctx, inv->replaced);
							VEC_CAT(&inv->replaced->tokens, &VEC_ITEM(&inv->in_args_expanded, i)->tokens);
						}
					
					}
					else if(bt->text == _va_narg) {
						inject_number(tu, ctx, inv->replaced, vararg_count);
					}
					else {
						VEC_PUSH(&inv->replaced->tokens, bt);
					}
				}
			}
		}
		else if(bt->text == _va_narg) {
			inject_number(tu, ctx, inv->replaced, vararg_count);
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
					
					inject_stringified(tu, ctx, inv->replaced, VEC_ITEM(&inv->in_args, ani));
				
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
	
//	printf("  -- final rescan --\n");
	inv->output = expand_token_list(tu, ctx, inv->replaced);


	// TODO: mark macro disabled


	return;
}


void undef_macro(cpp_tu_t* tu, char* name) {
	cpp_macro_name_t* mn;
	
	if(!HT_get(&tu->macros, name, &mn)) {
//		printf("  undefining %s\n", name);
		VEC_FREE(&mn->defs);
		free(mn);
	
		HT_delete(&tu->macros, name);
	}
	
//	if(ctx->parent) undef_macro(ctx->parent, name);
}







