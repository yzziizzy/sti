#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "lexer.h"



static void got_token(lexer_token_t* t) {
	if(t->is_whitespace) {
		/*printf("s: x%ld [", t->text_len);
		for(int i = 0; i < t->text_len; i++) {
			printf("%d ", t->text[i]);
		}
		printf("]\n");
		*/
	}
	else {
		printf("[%.*s]",  (int)t->text_len, t->text);
//		printf("[%ld,%ld] t:%.*s, ", t->start_line, t->start_col, (int)t->text_len, t->text);
	}
	if(t->eol) printf("\n");
}




int main(int argc, char* argv[]) {

	
	lexer_opts_t opts;
	opts.symbols = (char*[]){
		"(", ")", "[", "]", "{", "}",
		".", "...", ",", ";",
		"=", "==", "->",
		"-", "+", "/", "%", "*",
		"-=", "+=", "/=", "%=", "*=",
		">=", ">", "<=", "<", 
		"<<", "<<=", ">>", ">>=",
		"&&", "||",
		"~", "!", "!=", "^", "|", "&",
		"~=", "!=", "^=", "|=", "&=",
		"++", "--", 
		"?", ":",
		"#", "##",
		"<%", "%>", "<:", ":>", "%:", "%:%:",
		NULL
	};
	
	opts.got_token = got_token;
	
	lex_file("./lexer.c", &opts);

	return 0;
}










