#include <stdlib.h>
#include <stdio.h>


#include "lexer.h"
#include "cpp.h"
#include "parser.h"


int main(int argc, char* argv[]) {
	
	
	cpp_tu_t* tu = calloc(1, sizeof(*tu));
	// need to pre-define __X86_64__
	
//	VEC_PUSH(&tu->system_inc_dirs, "/");
	VEC_PUSH(&tu->system_inc_dirs, "/usr/include/");
	VEC_PUSH(&tu->system_inc_dirs, "/usr/lib/gcc/x86_64-pc-linux-gnu/11.2.1/include/");
	
	preprocess_file(tu, NULL, "./sample.c", 0);
//	preprocess_file(tu, NULL, "/usr/include/stdint.h", 1);
	
	
	ast_tu_t* atu = calloc(1, sizeof(*atu));
	atu->cpp = tu;

	c_parser_tu(tu, atu);
	
	
	
	/*
	cpp_context_t* ctx = tu->root_ctx;
	
	printf("\noutput:\n");
	int was_ws = 0;
	VEC_EACH(&ctx->out->tokens, i, t) {
		if(t->type == LEXER_TOK_COMMENT) {}
		else if(t->type == LEXER_TOK_SPACE) {
			if(was_ws) continue;
			printf(" ");
			was_ws = 1;
			if(t->has_newline) printf("\n");
		}
		else {
			printf("%s ", t->text);
		
			was_ws = 0;
			if(t->has_newline) printf("\n");
		}
	}
	printf("\n");
	*/
	return 0;
}



