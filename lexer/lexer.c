#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../fs.h"
#include "../vec.h"



#include "lexer.h"



enum {
	FLAG_NONE = 0,
	FLAG_SL_COMMENT,
	FLAG_ML_COMMENT_OPEN,
};

struct lexer_info;
typedef struct lexer_info lexer_info_t;

typedef int (*handler_fn)(lexer_info_t*);

typedef struct prefix_node {
	int c;
	int flags;
	void* id;
	VEC(handler_fn) handlers;
	struct prefix_node* sibling; 
	struct prefix_node* kids;
} prefix_node_t;


typedef struct prefix_tree {
	prefix_node_t root;
} prefix_tree_t;



typedef struct lexer_info {
	void (*got_token)(lexer_token_t*);
	lexer_token_t cached_token;
	char have_cached_token;
	
	char* src, *src_end;
	size_t src_len;
	
	char* head;
	char* line_start;
	long line_num;
	long col_num;
	
	int c;
	
	struct {
		long line_num, col_num;
	}* num_buffer;
	char* cached_buffer;
	char* buffer;
	size_t buf_alloc;
	size_t buf_len;
	
	char eof;
	char c_sol, c_eol;
	char t_sol, t_eol;
	char was_space;
	char in_generic_token;
	
	prefix_node_t* cur_node;
	
} lexer_info_t;







void read_char(lexer_info_t* lx) {
	
RESTART:
	
	// check if there's anything left to read
	if(lx->head >= lx->src_end) {
		lx->c = 0;
		lx->eof = 1;
		
		return;
	}
	
	// count lines
	if(lx->c_sol) lx->c_sol = 0;
	if(lx->c_eol) {
		lx->line_start = lx->head;
		lx->line_num++;
		lx->c_eol = 0;
		lx->c_sol = 1;
	}

	int c = lx->head[0];
	lx->col_num = lx->head - lx->line_start;
	
	if(c == '?' &&
		// check for fucking trigraphs.
		lx->src_end >= lx->head + 2 &&
			 lx->head[1] == '?') {
				#define FUCK_TRIGRAPHS(a,b) if(lx->head[2] == a) { c = b; goto FUCKING_FINALLY; }
			
				FUCK_TRIGRAPHS('/', '\\')
				FUCK_TRIGRAPHS('=', '#')
				FUCK_TRIGRAPHS('\'', '^')
				FUCK_TRIGRAPHS('(', '[')
				FUCK_TRIGRAPHS(')', ']')
				FUCK_TRIGRAPHS('!', '|')
				FUCK_TRIGRAPHS('<', '{')
				FUCK_TRIGRAPHS('>', '}')
				FUCK_TRIGRAPHS('-', '~')
				
				lx->head++;
				
				if(0) {
				FUCKING_FINALLY:
					lx->head += 3;
				}
			
		
	}
	else lx->head++;
	
	
	// checked for escaped newlines
	if(c == '\\') {
		if(lx->src_end > lx->head) {
			if(lx->head[0] == '\r') {
				if(lx->src_end > lx->head + 1 && lx->head[1] == '\n') // because Windows is like a mechanical typewriter but slower
					lx->head += 2;
				else 
					lx->head += 1;
				
				lx->line_num++;
				
				goto RESTART;
			}
			else if(lx->head[0] == '\n') {
				if(lx->src_end > lx->head + 1 && lx->head[1] == '\r') 
					lx->head += 2;
				else
					lx->head += 1;
				
				lx->line_num++;
				
				goto RESTART;
			}
		}
	}
	
	// check for regular newlines
	else if(c == '\r') {
		if(lx->src_end > lx->head && lx->head[0] == '\n') {
			lx->head++;
		}
		
		lx->c_eol = 1;
		c = '\n'; // normalize newlines
	}	
	else if(c == '\n') {
		if(lx->src_end > lx->head && lx->head[0] == '\r') {
			lx->head++;
		}
		
		lx->c_eol = 1;
	}	
	
	lx->c = c;
	
	
	
	// push c into the buffer
	if(lx->buf_alloc <= lx->buf_len) {
		lx->buf_alloc *= 2;
		lx->buffer = realloc(lx->buffer, lx->buf_alloc * sizeof(*lx->buffer));
		lx->cached_buffer = realloc(lx->cached_buffer, lx->buf_alloc * sizeof(*lx->cached_buffer));
		lx->num_buffer = realloc(lx->num_buffer, lx->buf_alloc * sizeof(*lx->num_buffer));
	} 
	
	lx->buffer[lx->buf_len] = c;
	lx->num_buffer[lx->buf_len].line_num = lx->line_num;
	lx->num_buffer[lx->buf_len].col_num = lx->col_num;
	lx->buf_len++;
}


static void shift_buffer(lexer_info_t* lx, int offset) {
	if(offset == 0) {
		lx->buf_len = 0;
		return;
	}
	
	memmove(lx->buffer, lx->buffer + lx->buf_len - offset, offset * sizeof(*lx->buffer)); 
	memmove(lx->num_buffer, lx->num_buffer + lx->buf_len - offset, offset * sizeof(*lx->num_buffer)); 
	lx->buf_len = offset;
}



void prefix_node_add_string(prefix_node_t* n, char* s, void* id) {
	if(s[0] == 0) {
		n->id = id;
		return;
	}
	
	prefix_node_t* k = n->kids;
	while(k && k->c != s[0]) k = k->sibling;
	
	if(!k) {
		k = calloc(1, sizeof(*k));
		k->sibling = n->kids;
		n->kids = k;
		k->id = 0;
		k->c = s[0];
	}
	
	prefix_node_add_string(k, s + 1, id); 
}


void prefix_tree_add_string(prefix_tree_t* tree, char* s, void* id) {
	prefix_node_add_string(&tree->root, s, id);
}


void prefix_node_add_handler(prefix_node_t* n, char* preamble, handler_fn handler) {
	if(preamble[0] == 0) {
		VEC_PUSH(&n->handlers, handler);
		return;
	}
	
	prefix_node_t* k = n->kids;
	while(k && k->c != preamble[0]) k = k->sibling;
	
	if(!k) {
		k = calloc(1, sizeof(*k));
		k->sibling = n->kids;
		n->kids = k;
		k->c = preamble[0];
	}
	
	prefix_node_add_handler(k, preamble + 1, handler); 
}

void prefix_tree_add_handler(prefix_tree_t* tree, char* preamble, handler_fn handler) {
	prefix_node_add_handler(&tree->root, preamble, handler);
}


static void send_token(lexer_info_t* lx, size_t len, char type) {
	
	if(type == -1) {
		if(lx->was_space) type = LEXER_TOKEN_TYPE_whitespace;
		else type = LEXER_TOKEN_TYPE_unknown;
	}
	
	lexer_token_t tok = {
		.start_line = lx->num_buffer[0].line_num,
		.start_col = lx->num_buffer[0].col_num,
		.end_line = lx->num_buffer[len-1].line_num,
		.end_col = lx->num_buffer[len-1].col_num,
		.text = lx->cached_buffer,
		.text_len = len,
		.type = type,
		.is_generic = lx->in_generic_token,
		.id = lx->cur_node->id,
		.sol = lx->t_sol,
		.eol = lx->t_eol,
	};
	
	if(lx->have_cached_token) {
		if((lx->was_space && lx->t_eol) || lx->t_sol) {
			lx->cached_token.eol = 1;
		}
		
		lx->got_token(&lx->cached_token);
	}
	
	lx->have_cached_token = 1;
	lx->cached_token = tok;
	memcpy(lx->cached_buffer, lx->buffer, lx->buf_len);
	
	// translate digraphs at the token level
	if(len == 2) {
		if(lx->buffer[0] == '<') {
			     if(lx->buffer[1] == ':') { lx->cached_buffer[0] = '['; lx->cached_token.text_len = 1; }
			else if(lx->buffer[1] == '%') { lx->cached_buffer[0] = '{'; lx->cached_token.text_len = 1; }
		}
		else if(lx->buffer[0] == '%') {
			     if(lx->buffer[1] == '>') { lx->cached_buffer[0] = '}'; lx->cached_token.text_len = 1; }
			else if(lx->buffer[1] == ':') { lx->cached_buffer[0] = '#'; lx->cached_token.text_len = 1; }
		}
		else if(lx->buffer[0] == ':') {
			if(lx->buffer[1] == '>') { lx->cached_buffer[0] = ']'; lx->cached_token.text_len = 1; }
		}
	}
	else if(len == 4) {
		if(lx->buffer[0] == '%' && lx->buffer[1] == ':' && lx->buffer[2] == '%' && lx->buffer[3] == ':') {
			lx->cached_buffer[0] = '#'; lx->cached_buffer[1] = '#'; lx->cached_token.text_len = 2;
		}
	}
	
	
	if(!lx->was_space) lx->t_sol = 0;
	lx->t_eol = 0;
}


int lex_pp(lexer_info_t* lx) {
	while(1) {
		read_char(lx);
		if(lx->eof) break;
		if(lx->c == '\n') {
			send_token(lx, lx->buf_len - 1, LEXER_TOKEN_TYPE_generic);
			shift_buffer(lx, 1);
			lx->was_space = 1;
			break;
		}
	}
	
	return 0;
}

int lex_num(lexer_info_t* lx) {
	// the C preprocessor has a strange definition of numbers:
	//  An optional . followed by a decimal number, followed by amy sequence of letters, numbers, +, - or .
	
	while(1) {
		read_char(lx);
		if(lx->eof) break;
		
		int c = lx->c;
		
		if(!(isalnum(c) || c == '.' || c == '-' || c == '+')) {
			// done
			send_token(lx, lx->buf_len - 1, LEXER_TOKEN_TYPE_number);
			shift_buffer(lx, 1);
			return 1;
		}
	}
	
	return 0;
}

int lex_dot(lexer_info_t* lx) {
	// dots need a degree of smart lookahead.
	// . = .
	// .. = ., .
	// ... = ...
	// .\d = number
	
	
	// just a . so far
	
	read_char(lx);
	if(lx->eof) return 0;
	
	if(lx->c != '.') {
		if(isdigit(lx->c)) {
			return lex_num(lx);
		}
		
		send_token(lx, lx->buf_len - 1, LEXER_TOKEN_TYPE_punct);
		shift_buffer(lx, 1);
		return 1;
	}
	
	// .. now

	read_char(lx);
	if(lx->eof) return 0;
	
	if(lx->c != '.') {
		// send two separate dots
		send_token(lx, 1, LEXER_TOKEN_TYPE_punct);
		shift_buffer(lx, lx->buf_len - 1);
		send_token(lx, 1, LEXER_TOKEN_TYPE_punct);
		shift_buffer(lx, lx->buf_len - 1);
		return 1;
	}
	else {
		// send ...
		send_token(lx, lx->buf_len, LEXER_TOKEN_TYPE_punct);
		shift_buffer(lx, 0);
		return 0;
	}
}

int lex_slc(lexer_info_t* lx) {
	
	while(1) {
		read_char(lx);
		if(lx->eof) break;
		if(lx->c == '\n') {
			send_token(lx, lx->buf_len - 1, LEXER_TOKEN_TYPE_comment);
			shift_buffer(lx, 1);
			lx->was_space = 1;
			return 1;
		}
	}
	
	return 0;
}

int lex_mlc(lexer_info_t* lx) {
	
	while(1) {
		read_char(lx);
		if(lx->eof) break;
		if(lx->c == '/' && lx->buffer[lx->buf_len - 2] == '*') {
			send_token(lx, lx->buf_len, LEXER_TOKEN_TYPE_comment);
			shift_buffer(lx, 0);
			break;
		}
	}
	
	return 0;
}

int lex_string(lexer_info_t* lx) {
	int escaped = 0;
	
	while(1) {
		read_char(lx);
		if(lx->eof) break;
		
		if(escaped) escaped = 0;
		else if(lx->c == '\\') escaped = 1;
		else if(lx->c == '"') {
			send_token(lx, lx->buf_len, LEXER_TOKEN_TYPE_stringlit);
			shift_buffer(lx, 0);
			break;
		}
	}
	
	return 0;
}

int lex_charlit(lexer_info_t* lx) {
	int escaped = 0;
	
	while(1) {
		read_char(lx);
		if(lx->eof) break;
		
		if(escaped) escaped = 0;
		else if(lx->c == '\\') escaped = 1;
		else if(lx->c == '\'') {
			send_token(lx, lx->buf_len, LEXER_TOKEN_TYPE_charlit);
			shift_buffer(lx, 0);
			break;
		}
	}
	
	return 0;
}


int lex_file(char* path, lexer_opts_t* opts) {
	size_t fsz;
	
	char* contents = readWholeFile(path, &fsz);
	if(!contents) return -1;
	
	lexer_token_t tok;
	
	lexer_info_t* lx = calloc(1, sizeof(*lx));
	lx->got_token = opts->got_token;
	lx->src = contents;
	lx->head = lx->src;
	lx->src_len = fsz;
	lx->src_end = contents + fsz - 1;
	
	lx->buf_alloc = 256;
	lx->buffer = calloc(1, lx->buf_alloc * sizeof(*lx->buffer));
	lx->cached_buffer = calloc(1, lx->buf_alloc * sizeof(*lx->cached_buffer));
	lx->num_buffer = calloc(1, lx->buf_alloc * sizeof(*lx->num_buffer));
	
	
	// initialize a few things
	lx->line_num = 1;
	lx->line_start = lx->src;
	
	// create a prefix tree of symbols to walk it easier
	prefix_tree_t tree = {0};
	
	char** sp = opts->symbols;
	for(; *sp; sp++) {
		prefix_tree_add_string(&tree, *sp, 1);
	}
	
	//prefix_tree_add_handler(&tree, "#", lex_pp);
	prefix_tree_add_handler(&tree, ".", lex_dot);
	prefix_tree_add_handler(&tree, "0", lex_num);
	prefix_tree_add_handler(&tree, "1", lex_num);
	prefix_tree_add_handler(&tree, "2", lex_num);
	prefix_tree_add_handler(&tree, "3", lex_num);
	prefix_tree_add_handler(&tree, "4", lex_num);
	prefix_tree_add_handler(&tree, "5", lex_num);
	prefix_tree_add_handler(&tree, "6", lex_num);
	prefix_tree_add_handler(&tree, "7", lex_num);
	prefix_tree_add_handler(&tree, "8", lex_num);
	prefix_tree_add_handler(&tree, "9", lex_num);
	prefix_tree_add_handler(&tree, "//", lex_slc);
	prefix_tree_add_handler(&tree, "/*", lex_mlc);
	prefix_tree_add_handler(&tree, "\"", lex_string);
	prefix_tree_add_handler(&tree, "'", lex_charlit);
	
	
	lx->cur_node = &tree.root;
	
	
	lx->was_space = 0;
	lx->in_generic_token = 0;
	
	// read the file one painfully-slow logical character at a time
	while(1) {
		read_char(lx);
		
	FULL_RETRY:
		if(lx->c_sol) lx->t_sol = 1; 
		if(lx->c_eol) lx->t_eol = 1; 
		
		// end of file handling
		if(lx->eof) {			
			if(lx->buf_len > 0) {
				lx->t_eol = 1;
//				printf("A");
				send_token(lx, lx->buf_len, -1);
			}
			
			if(lx->have_cached_token) {
				lx->cached_token.eol = 1;
				lx->got_token(&lx->cached_token);
			}
			
			break;
		};
		

		int c = lx->c;
		
		if(isspace(c)) {
			// try to end the token
			if(!lx->was_space && lx->buf_len > 1) {
//				printf("B(%d)", c);
				send_token(lx, lx->buf_len - 1, -1);
				
				shift_buffer(lx, 1);
				lx->in_generic_token = 0;
			}
			
			lx->was_space = 1;
		}
		else {
			if(lx->was_space && lx->buf_len > 1) {
//				printf("C");
				send_token(lx, lx->buf_len - 1, -1);
				shift_buffer(lx, 1);
				lx->was_space = 0;
				lx->cur_node = &tree.root;
			}
		
		REDO:
			prefix_node_t* n = lx->cur_node->kids;
			while(n) {
				if(n->c == c) {
					lx->cur_node = n;
					goto FOUND;
				}
				
				n = n->sibling;
			}
			
			// NOT FOUND

			// see if the current node was terminal and shift
			if(lx->cur_node->id) {
//				printf("D");
				send_token(lx, lx->buf_len - 1, -1);
				
				shift_buffer(lx, 1);
				lx->in_generic_token = 0;
				lx->cur_node = &tree.root;
				goto REDO;
			}
			else {
				lx->in_generic_token = 1;
			}
			
			lx->cur_node = &tree.root;
		}
		
		continue;
		
	FOUND:
		if(lx->in_generic_token && lx->buf_len > 0) {
//			printf("E(%c)", c);
			send_token(lx, lx->buf_len - 1, -1);
		
			shift_buffer(lx, 1);
		}
		
		lx->in_generic_token = 0;
		
		// check for special handlers
		if(VEC_LEN(&lx->cur_node->handlers)) {
			VEC_EACH(&lx->cur_node->handlers, i, h) {
				if(!h(lx)) {
					lx->cur_node = &tree.root;
				}
				else {
					lx->cur_node = &tree.root;
					goto FULL_RETRY;
				}
			}
		}
		
		
	}

	return 0;
}






















