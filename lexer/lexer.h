#ifndef __sti__lexer_lexer_h__
#define __sti__lexer_lexer_h__




typedef struct lexer_token {
	long start_line, start_col;
	long end_line, end_col;
	
	char* text;
	size_t text_len;
	
	char sol, eol;
	char is_generic;
	char is_whitespace;
	void* id;
	
} lexer_token_t;




typedef struct lexer_opts {
	void (*got_token)(lexer_token_t*);
	
	char** symbols;
	
} lexer_opts_t;






int lex_file(char* path, lexer_opts_t* opts);











#endif
