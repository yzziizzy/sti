#ifndef __sti__lexer_lexer_h__
#define __sti__lexer_lexer_h__


#define LEXER_TYPE_LIST(X) \
	X(unknown) \
	X(whitespace) \
	X(ident) \
	X(charlit) \
	X(stringlit) \
	X(number) \
	X(comment) \
	X(punct) \


enum {
	#define X(a,...) LEXER_TOKEN_TYPE_##a,
		LEXER_TYPE_LIST(X)
	#undef X
};



typedef struct lexer_token {
	long start_line, start_col;
	long end_line, end_col;
	
	char* text; // interred string, used as the token type
	size_t text_len;
	
	char sol, eol;
	char type;
	char is_generic;
	void* id; // doesn't appear to be used...
	
} lexer_token_t;




typedef struct lexer_opts {
	void (*got_token)(lexer_token_t*);
	
	char** symbols;
	
} lexer_opts_t;






int lex_file(char* path, lexer_opts_t* opts);











#endif
