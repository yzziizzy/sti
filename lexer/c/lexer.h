



#define LEXER_TOKEN_TYPE_LIST \
	X(INVALID) \
	X(COMMENT) \
	X(NUMBER) \
	X(SPACE) \
	X(CONST) \
	X(IDENT) \
	X(PUNCT) \
	X(STRING) \


enum {
#define X(a) LEXER_TOK_##a,
	LEXER_TOKEN_TYPE_LIST
#undef X
};

extern char* lexer_token_type_names[];


typedef struct lexer_source {
	size_t len;
	char* text;
	char* head;
} lexer_source_t;


typedef struct lexer_token {
	int type;
	char has_newline; // lexically speaking. escaped newlines don't count.
	
	int start_line;
	int start_col;
	
	int end_line;
	int end_col;
	
	size_t source_adv; // number of bytes consumed from the input
	size_t len; // length of the output token
	size_t alloc; // allocated size of the token buffer
	char* text; // the token text
} lexer_token_t;




int is_token(lexer_source_t* src, lexer_token_t* t);
int is_token_slow(lexer_source_t* src, lexer_token_t* t);
