#include <stdlib.h>
#include <stdio.h>


#include "lexer.h"
#include "cpp.h"


int main(int argc, char* argv[]) {
	
	
	cpp_tu_t* tu = calloc(1, sizeof(*tu));
	// need to pre-define __X86_64__
	
//	VEC_PUSH(&tu->system_inc_dirs, "/");
	VEC_PUSH(&tu->system_inc_dirs, "/usr/include/");
	
	preprocess_file(tu, NULL, "./sample.c", 0);
//	preprocess_file(tu, NULL, "/usr/include/stdint.h", 1);
	
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
	
	return 1;
	
	lexer_token_t t = {0};
	t.alloc = 256;
	t.text = malloc(t.alloc * sizeof(*t.text));
	
	lexer_source_t src = {0};
	src.text = ".x345";
	src.text = "/*fo*o*\\\n/";
	src.text = "//foo\\\nbar";
	src.text = "...";
	src.text = "..";
	src.text = "..\\\n.";
	src.text = "\\\n.";
	src.text = ".0";
	src.text = ".\\\nx0";
	src.text = ".x0";

	src.head = src.text;
	is_token(&src, &t);
	
	printf("%s: '%.*s'\n", lexer_token_type_names[t.type], (int)t.len, t.text);
}
