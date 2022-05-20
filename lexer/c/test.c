#include <stdlib.h>
#include <stdio.h>


#include "lexer.h"


int main(int argc, char* argv[]) {
	
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
