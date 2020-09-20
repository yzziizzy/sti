#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "assembler.h"









static int eatchar(struct Lexer* st, int c) {

	#include "./lexer_csets.c"
	
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

		#include "./lexer_switch.c"
		
		
		default: 
			printf("Lexer reached default: %d\n", st->state);
			st->state = LST_NULL; 
			return 0;
	}
	
	assert(0);
	// never gets here
ERROR:
	printf("Lexer error at line %d:%d: state %d(%s) %d='%c' \n", st->linenum, st->charnum, st->state, lexer_state_names[st->state], c, c);
	exit(1);
	
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



Lexer* start_lexer(char* source, size_t len) {
	Lexer* ls = calloc(1, sizeof(*ls));
	
	*ls = (Lexer){
		.state = 0,
		.balloc = 256,
		.blen = 0,
		.buffer = calloc(1, 256),
		
		.state = LST_NULL,
		.linenum = 0,
		.charnum = 0,
		
		.tokenState = 0,
		.tokenFinished = 0,
		
		.source = source,
		.sourceLen = len,
		.i = 0,
	};
}


int next_token(Lexer* ls) {
	
	ls->tokenFinished = 0;
	ls->state = LST_NULL;
	ls->blen = 0;
	
	for(int i = ls->i; ls->source[i];) {
		int ret;
		ret = eatchar(ls, ls->source[i]);
		
		if(ls->tokenFinished) { 
			// token is ready
			ls->buffer[ls->blen] = 0;
			ls->i = i;
			return 1;
		}
		
		if(ret) {
			i++; // advance on ret == 1
		}
	}
	
	return 0;
}


