#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>



enum LexState {
	LST_NULL = 0,
	LST_INVALID,
	
	#include "./parser_example_enums.h"
	
	LST_MAXVALUE
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

#define push_char_id(_state) \
do { \
	st->state = _state; \
	goto PUSH_CHAR_RET; \
} while(0)


#define discard_id(_state) \
do { \
	st->state = _state; \
	return 1; \
} while(0)


#define retry_as(_state) \
do { \
	st->state = _state; \
	goto RETRY; \
} while(0);
	
	// hopefully this works
	st->charnum++;
	if(c == '\n') {
		st->linenum++;
		st->charnum = 0;
	}
	

RETRY:
	switch(st->state) {

		#include "./parser_example_switch.c"
		
		
		default: 
			printf("Lexer reached default\n");
			return 0;
	}
	
	assert(0);
	// never gets here
ERROR:
	st->state = LST_NULL; 
	st->blen = 0;
	printf("Lexer error\n");
	return 1;

TOKEN_DONE:
	st->tokenFinished = 1;
	st->tokenState = st->state;
	return 0;
	
PUSH_CHAR_RET: 
	st->buffer[st->blen] = c; 
	st->blen++;
	return 1; 


}




int main(int argc, char* argv[]) {
	
	char* source = "func int8 nfuib uint77 ";
	
	struct lexer_state ls = {
		.state = 0,
		.balloc = 256,
		.blen = 0,
		.buffer = calloc(1, 256),
		
		.linenum = 0,
		.charnum = 0,
		
		.tokenState = 0,
		.tokenFinished = 0,
	};
	
	for(int i = 0; source[i];) {
		int ret;

// 				printf(" eating char: %c\n", line[i]);
		ret = eatchar(&ls, source[i]);
		
// 			printf(" post-state: %s\n", stateNames[ls.state]);
		
		if(ls.tokenFinished) { 
			// token is ready
			
			printf("got token: #%d '%.*s'\n", ls.tokenState, ls.blen, ls.buffer);
			
			// reset the lex state when done reading
			ls.tokenFinished = 0;
			ls.state = LST_NULL;
			ls.blen = 0;
			
		}
		
		if(ret) {
			i++; // advance on ret == 1
		}
	}
	
	return 0;
}


