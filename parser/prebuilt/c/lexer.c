#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "../../../sti.h"
#include "lexer.h"


char* state_names[] = {
	[LST_INVALID] = "LST_INVALID",
	#define PARSER_INCLUDE_ENUM_NAMES
	#include "./parser_example_generated.h"
	#undef PARSER_INCLUDE_ENUM_NAMES
	[LST_MAXVALUE] = "LST_MAXVALUE",
};


#define PARSER_INCLUDE_TERMINAL_DATA_DEFS
#include "./parser_example_generated.h"
#undef PARSER_INCLUDE_TERMINAL_DATA_DEFS

char** state_data[] = {
	#define PARSER_INCLUDE_TERMINAL_DATA
	#include "./parser_example_generated.h"
	#undef PARSER_INCLUDE_TERMINAL_DATA
};

// this is for the incremental lexing of each token, not the whole stream
struct lexer_state {
	enum LexState state;
	char* buffer;
	int blen;
	int balloc;
	
	int linenum;
	int charnum;
	
	enum LexState tokenState;
	int tokenFinished; // buffer should be consumed and cleaned at this point 
};





static int eatchar(struct lexer_state* st, int c) {
	#define PARSER_INCLUDE_CSETS
	#include "./parser_example_generated.h"
	#undef PARSER_INCLUDE_CSETS
	
#define push_char_id(_state) \
do { \
	st->state = _state; \
	goto PUSH_CHAR_RET; \
} while(0)


#define discard_char_id(_state) \
do { \
	st->state = _state; \
	return 1; \
} while(0)


#define retry_as(_state) \
do { \
	st->state = _state; \
	goto RETRY; \
} while(0);

#define done_zero_move(_state) \
do { \
	st->state = _state; \
	goto TOKEN_DONE; \
} while(0);

#define push_char_done(_state) \
do { \
	st->state = _state; \
	goto PUSH_CHAR_DONE; \
} while(0);

#define charset_has(cs, c) (c <= cs##_len && !!cs[c])

	// hopefully this works
	st->charnum++;
	if(c == '\n') {
		st->linenum++;
		st->charnum = 0;
	}
	

RETRY:
	switch(st->state) {
		#define PARSER_INCLUDE_SWITCH
		#include "./parser_example_generated.h"
		#undef PARSER_INCLUDE_SWITCH
		
		default: 
			printf("Lexer reached default: %d\n", st->state);
			st->state = LST_NULL; 
			return 0;
	}
	
	assert(0);
	// never gets here
ERROR:
	//printf("Lexer error at line %d:%d: state %d(%s) %d='%c' \n", st->linenum, st->charnum, st->state, state_names[st->state], c, c);
	st->state = LST_NULL; 
	st->blen = 0;
	return 1;

TOKEN_DONE:
	st->tokenFinished = 1;
	st->tokenState = st->state;
	return 0;
	
PUSH_CHAR_RET: 
	st->buffer[st->blen] = c; 
	st->blen++;
	return 1;
	
PUSH_CHAR_DONE: 
	st->buffer[st->blen] = c; 
	st->blen++;
	
	st->tokenFinished = 1;
	st->tokenState = st->state;
	return 1; 


}





TokenStream* LoadAndLexFile(char* path) {

	char* source = readWholeFile(path, NULL);
	
	TokenStream* ts = calloc(1, sizeof(*ts));
	
	struct lexer_state ls = {
		.state = 0,
		.balloc = 256,
		.blen = 0,
		.buffer = calloc(1, 256),
		
		.state = LST_NULL,
		.linenum = 0,
		.charnum = 0,
		
		.tokenState = 0,
		.tokenFinished = 0,
	};
	
	for(int i = 0; source[i];) {
		int ret;
		ret = eatchar(&ls, source[i]);
		
		if(ls.tokenFinished) { 
			// token is ready
			VEC_INC(&ts->tokens);
			LexerToken* t = &VEC_TAIL(&ts->tokens);
			
			t->tokenState = ls.tokenState;
			t->tokenText = strndup(ls.buffer, ls.blen);
			t->line = ls.linenum;
			t->character = ls.charnum;
			t->sourceFile = NULL;
			
// 			printf("got token: #%d (%s) '%.*s'\n", ls.tokenState, state_names[ls.tokenState], ls.blen, ls.buffer);
			
			// reset the lex state when done reading
			ls.tokenFinished = 0;
			ls.state = LST_NULL;
			ls.blen = 0;
		}
		
		if(ret) {
			i++; // advance on ret == 1
		}
	}

// 	printf("last token: #%d (%s) '%.*s'\n", ls.tokenState, state_names[ls.tokenState], ls.blen, ls.buffer);
	
	free(source);
	
	return ts;
}


