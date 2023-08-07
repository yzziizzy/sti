#include <stdio.h>

#include "parser_internal.h"







void cp_ctx_destroy_state(stack_state_t* st) {
	VEC_FREE(&st->type_specs);
	VEC_FREE(&st->type_names);
	VEC_FREE(&st->idents);
	VEC_FREE(&st->array_dims);
	st->id = 0;
}



ast_type_t* cp_ctx_process_type(cp_ctx_t* ctx) {
	ast_type_t* t = calloc(1, sizeof(*t));
	
	
	#define X(a, ...) int has_##a = 0;
		PARSER_TYPESPEC_LIST(X)
		X(struct)
		X(enum)
		X(union)
	#undef X
	
	VEC_EACH(&ctx->state->type_specs, i, tok) {
		#define X(a, ...) if(tok->text == ctx->_##a) has_##a++;
			PARSER_TYPESPEC_LIST(X)
			X(struct)
			X(enum)
			X(union)
		#undef X
		
		#define X(a, ...) if(tok->text == ctx->_##a) t->specs |= TYPESPEC_##a;
			PARSER_TYPESPEC_LIST(X)
		#undef X
	}
	
	
	// TODO: check duplicates, invalid combos
	ast_typename_t* typename = NULL;
	
	if(VEC_LEN(&ctx->state->type_names) > 1) {
		printf("too many type names\n");
	}
	else if(VEC_LEN(&ctx->state->type_names) == 0) {
		if(has_union || has_struct) {
			printf("%s type\n", has_struct ? "struct" : "union");
		}
		else {
			printf("missing type name\n");
		}
	}
	else {
		typename = &VEC_ITEM(&ctx->state->type_names, 0);
		t->type_name = typename->name;
	}
	
	return t;
}






