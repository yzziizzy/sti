

// TODO: ternary

// name     , arity, precedence, associativity (L = 0, R = 1)
#define OPERATOR_LIST \
	X(PLUS,      2, 40, 0) \
	X(MINUS,     2, 40, 0) \
	X(MUL,       2, 30, 0) \
	X(DIV,       2, 30, 0) \
	X(MOD,       2, 30, 0) \
	X(BIT_AND,   2, 80, 0) \
	X(BIT_OR,    2, 82, 0) \
	X(BIT_NOT,   1, 20, 1) \
	X(BIT_XOR,   2, 81, 0) \
	X(SHR,       2, 50, 0) \
	X(SHL,       2, 50, 0) \
	X(LOGIC_AND, 2, 90, 0) \
	X(LOGIC_OR,  2, 91, 0) \
	X(LOGIC_NOT, 1, 20, 1) \
	X(GT,        2, 60, 0) \
	X(LT,        2, 60, 0) \
	X(GTE,       2, 60, 0) \
	X(LTE,       2, 60, 0) \
	X(NEQ,       2, 70, 0) \
	X(EQ,        2, 70, 0) \
	X(TERN,      3, 92, 0) \
	X(COLON,     1, 92, 0) \
	X(UNARY_NEG, 1, 20, 1) \
	X(DEFINED,   1, 10, 1) \
	X(LPAREN,   -1, 127, 0) \
	X(RPAREN,   -1,  -1, 0) \


#define X(a, ...) OP_##a,
enum {
	OP_NONE = 0,
	OPERATOR_LIST
};
#undef X

#define X(a, ...) [OP_##a] = #a,
char* operator_names[] = {
	[OP_NONE] = "none",
	OPERATOR_LIST
};
#undef X

#define X(a, b, c, d) [OP_##a] = {OP_##a, b, c, d},
cpp_stack_token_t operator_data[] = {
	[OP_NONE] = {-1, 0, 127, 0},
	OPERATOR_LIST
};
#undef X



int is_defined(cpp_tu_t* tu, lexer_token_t* name) {
	return NULL != get_macro_def(tu, name);
}

int is_integer(lexer_token_t* t, long* val) {
	char* end;
	long l = strtol(t->text, &end, 0);
	if(end != t->text + strlen(t->text)) {
		if(val) *val = 0;
		return 0; // not a valid integer
	}
	
	if(val) *val = l;
	return 1;
}



int probe_operator_type(lexer_token_t* t) {
	char* s = t->text;
	int type = OP_NONE;
	
	switch(s[0]) {
		case '=':
			switch(s[1]) {
				case '=': type = OP_EQ; break;
			}
			break;
		case '<':
			switch(s[1]) {
				case '<': type = OP_SHL; break;
				case '=': type = OP_LTE; break;
				default: type = OP_LT; break;
			}
			break;
		case '>':
			switch(s[1]) {
				case '>': type = OP_SHR; break;
				case '=': type = OP_GTE; break;
				default: type = OP_GT; break;
			}
			break;
		case '!':
			switch(s[1]) {
				case '=': type = OP_NEQ; break;
				default: type = OP_LOGIC_NOT; break;
			}
			break;
		case '~': type = OP_BIT_NOT; break;
		case '^': type = OP_BIT_XOR; break;
		case '|':
			switch(s[1]) {
				case '|': type = OP_LOGIC_OR; break;
				default: type = OP_BIT_OR; break;
			}
			break;
		case '&':
			switch(s[1]) {
				case '&': type = OP_LOGIC_AND; break;
				default: type = OP_BIT_AND; break;
			}
			break;
		case '/': type = OP_DIV; break;
		case '*': type = OP_MUL; break;
		case '%': type = OP_MOD; break;
		case '-': type = OP_MINUS; break;
		case '+': type = OP_PLUS; break;
		case '?': type = OP_TERN; break;
		case ':': type = OP_COLON; break;
		case ')': type = OP_RPAREN; break;
		case '(': type = OP_LPAREN; break;
	}
	
	return type;
}


void reduce(cpp_tu_t* tu, cpp_context_t* ctx) {
	int op;
	/*
	printf("!reducing:\n");
	
	VEC_EACH(&ctx->value_stack, vi, v) {
		printf("     #%ld - %s\n", vi, v->text);
	}
	*/
	VEC_POP(&ctx->oper_stack, op);
	
	long lval, rval, tval, res = 0;
	lexer_token_t* ltok, *rtok, *ttok;
	
	if(operator_data[op].arity > 0) {
		VEC_POP(&ctx->value_stack, rtok);
		is_integer(rtok, &rval);
	}
	
	if(operator_data[op].arity > 1) {
		VEC_POP(&ctx->value_stack, ltok);
		is_integer(ltok, &lval);
	}
	
	if(operator_data[op].arity > 2) {
		VEC_POP(&ctx->value_stack, ttok);
		is_integer(ttok, &tval);
	}
	
	
	switch(op) {
		case OP_EQ: res = lval == rval; break;
		case OP_NEQ: res = lval != rval; break;
		case OP_GTE: res = lval >= rval; break;
		case OP_LTE: res = lval <= rval; break;
		case OP_GT: res = lval > rval; break;
		case OP_LT: res = lval < rval; break;
		case OP_PLUS: res = lval + rval; break;
		case OP_MINUS: res = lval - rval; break;
		case OP_MUL: res = lval * rval; break;
		case OP_BIT_NOT: res = ~rval; break;
		case OP_LOGIC_NOT: res = !rval; break;
		case OP_LOGIC_AND: res = lval && rval; break;
		case OP_LOGIC_OR: res = lval || rval; break;
		case OP_BIT_AND: res = lval & rval; break;
		case OP_BIT_OR: res = lval | rval; break;
		case OP_BIT_XOR: res = lval ^ rval; break;
		case OP_SHR: res = lval >> rval; break;
		case OP_SHL: res = lval << rval; break;
		case OP_UNARY_NEG: res = -rval; break;
		
		case OP_COLON:
//			VEC_PUSH(&ctx->value_stack, ttok);
//			VEC_PUSH(&ctx->value_stack, ltok);
			res = rval; //VEC_PUSH(&ctx->value_stack, rtok);
			break;
			
		case OP_TERN:
			res = tval ? lval : rval; 
			
			break;
		
		// these are just hacked because this parser doesn't handle logical operator shortcutting
		case OP_DIV: rval != 0 ? res = lval / rval : 0; break;
		case OP_MOD: rval != 0 ? res = lval % rval : 0; break;
		
		case OP_DEFINED:
			res = is_defined(tu, rtok);
			break;
		
		case OP_LPAREN:
//			printf("     evaluating lparen!!!!\n");
			return;
			
		case OP_RPAREN:
//			printf("     evaluating rparen!!!!\n");			
			return;
		
	}

//	printf(" %s - res %ld, tval: %ld, lval: %ld, rval: %ld\n", operator_names[op], res, tval, lval, rval);

	// BUG: this leaks...
	lexer_token_t* res_tok = malloc(sizeof(*res_tok));
	res_tok->type = LEXER_TOK_NUMBER;
	res_tok->text = sprintfdup("%ld", res);

	VEC_PUSH(&ctx->value_stack, res_tok);
}


long eval_exp(cpp_tu_t* tu, cpp_context_t* ctx, cpp_token_list_t* exp) {
	
	char* _defined = strint_(tu->str_table, "defined");
	
	int was_oper = 1;
	
	VEC_TRUNC(&ctx->oper_stack);
	VEC_TRUNC(&ctx->value_stack);
	
	
	VEC_EACH(&exp->tokens, ni, n) {
	
		if(n->type == LEXER_TOK_NUMBER || n->type == LEXER_TOK_IDENT) {
			VEC_PUSH(&ctx->value_stack, n);
//			printf("  pushing %s to value stack\n", n->text);
			was_oper = 0;
		}
		
		if(n->type == LEXER_TOK_PUNCT) {
			// check for operators, rest are 0
			
			int op = probe_operator_type(n);
			
			if(op == OP_LPAREN) {
				VEC_PUSH(&ctx->oper_stack, op);
//				printf("  pushing lparen to the stack\n");
				was_oper = 1;
			}
			else if(op == OP_RPAREN) {
//				printf("     executing rparen\n");
				do {
					int top = VEC_TAIL(&ctx->oper_stack);
//					printf("       - %s\n", operator_names[top]);
					
					if(top == OP_LPAREN) {
//						printf("       - found lparen, exiting loop (top value: --)\n" /*VEC_TAIL(&ctx->value_stack)*/);
						VEC_POP1(&ctx->oper_stack);
						break;
					}
					
					reduce(tu, ctx);
					
				} while(VEC_LEN(&ctx->oper_stack));
				
				was_oper = 0;
			}
			else if(op == OP_MINUS && was_oper) {
				VEC_PUSH(&ctx->oper_stack, OP_UNARY_NEG);
//				printf("  pushing operator UNARY_NEG to the stack\n");
			}
			else if(op > 0) {
				int top = 0;
				if(VEC_LEN(&ctx->oper_stack)) 
					top = VEC_TAIL(&ctx->oper_stack);
				
//				printf("top: %d, op: %d\n", top, op);
				if(operator_data[top].prec < operator_data[op].prec) {
					// if(top.prec == r->prec && r->assoc == STI_OP_ASSOC_LEFT) break;
					reduce(tu, ctx);
				}
				
				VEC_PUSH(&ctx->oper_stack, op);
//				printf("  pushing operator %s to the stack\n", operator_names[op]);
				was_oper = 1;
			}
			else {
				VEC_PUSH(&ctx->value_stack, n);
//				printf("  pushing 0 to the stack due to '%s'\n", n->text);
				was_oper = 0;
			}
		}
		
	}
	
	
	// finish off the operator stack
	while(VEC_LEN(&ctx->oper_stack)) {
		reduce(tu, ctx);
	}
	
	lexer_token_t* final_tok;
	long final = 0; 
	VEC_POP(&ctx->value_stack, final_tok);
	
	final = strtol(final_tok->text, NULL, 0);

	
	return final;
}

