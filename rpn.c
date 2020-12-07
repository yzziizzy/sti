// Public Domain

#include <math.h>
#include <ctype.h>

#include "err.h"
#include "rpn.h"
#include "vec.h"



int parse_arithmetic_string(char* src, char*** out, size_t* outlen) {
	VEC(char*) o;
	char* s = src;
	int l;
	
	VEC_INIT(&o);
	
	for(; *s; s++) {
		int c = *s;
		
		switch(c) {
			case ' ': continue;
			case '\n': continue;
			case '\r': continue;
			case '\t': continue;
			case '+':
				VEC_PUSH(&o, strdup("+"));
				break;
			case '*':
				if(s[1] == '*') {
					VEC_PUSH(&o, strdup("**"));
					s++;
				}
				else VEC_PUSH(&o, strdup("*"));
				break;
			case '/': VEC_PUSH(&o, strdup("/")); break;
			case '-':
				// TODO: check for negative signs
				VEC_PUSH(&o, strdup("-"));
				break;
			case '&': VEC_PUSH(&o, strdup("&")); break;
			case '^': VEC_PUSH(&o, strdup("^")); break;
			case '|': VEC_PUSH(&o, strdup("|")); break;
			case '~': VEC_PUSH(&o, strdup("~")); break;
			case '(': VEC_PUSH(&o, strdup("(")); break;
			case ')': VEC_PUSH(&o, strdup(")")); break;
			case '[': VEC_PUSH(&o, strdup("[")); break;
			case ']': VEC_PUSH(&o, strdup("]")); break;
			
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
				l = strspn(s, "0123456789.xbeE");
				VEC_PUSH(&o, strndup(s, l));
				s += l;
				break;
			
			default:
				continue;
		}
		
	}
	
	if(out) *out = o.data;
	if(outlen) *outlen = o.len;
	
	return 0;
}



static sti_op_prec_rule* shunting_classify(sti_op_prec_rule* rules, char* s) {
	sti_op_prec_rule* r = rules;
	
	for(; r->token; r++) {
		if(0 == strcmp(r->token, s)) return r;
	}
	
	return rules;
}


int infix_to_rpn(sti_op_prec_rule* rules, char** infix, char*** rpn, size_t* rpnlen) {
	
	VEC(sti_op_prec_rule) opstack;
	VEC(char*) out;
	
	VEC_INIT(&out);
	VEC_INIT(&opstack);
	
	char** n = infix;
	
	
	for(; *n; n++) {
		sti_op_prec_rule* r = shunting_classify(rules, *n);
		
		if(r == rules) {
			VEC_PUSH(&out, *n);
			continue;
		}
		
		if(r->assoc == STI_OP_OPEN_PAREN) {
			VEC_PUSH(&opstack, *r);
			continue;
		}
		
		if(r->assoc == STI_OP_CLOSE_PAREN) {
			while(VEC_LEN(&opstack) > 0) {
				sti_op_prec_rule top = VEC_TAIL(&opstack);
				
				if(top.assoc == STI_OP_OPEN_PAREN) {
					if(top.prec != r->prec) goto PAREN_MISMATCH;
					
					VEC_POP1(&opstack);
					goto END_PAREN;
				}
				
				sti_op_prec_rule o;
				VEC_POP(&opstack, o);
				VEC_PUSH(&out, o.token);
			}
			
			goto PAREN_MISMATCH;
			
		END_PAREN:
			continue;
		}
		
		
		// normal tokens
		while(VEC_LEN(&opstack) > 0) {
			
			sti_op_prec_rule top = VEC_TAIL(&opstack);
			
			if(top.assoc == STI_OP_OPEN_PAREN) break;
			if(top.prec < r->prec) break;
			if(top.prec == r->prec && r->assoc == STI_OP_ASSOC_LEFT) break;
			
			sti_op_prec_rule o;
			VEC_POP(&opstack, o);
			VEC_PUSH(&out, o.token);
		}
		
		VEC_PUSH(&opstack, ((sti_op_prec_rule){
			.token = *n, 
			.prec = r->prec, 
			.assoc = r->assoc, 
			.arity = r->arity 
		}));
		
	}
	
	
	// pop off the rest
	while(VEC_LEN(&opstack) > 0) {
		sti_op_prec_rule o;
		VEC_POP(&opstack, o);
		VEC_PUSH(&out, o.token);
	}
	
	VEC_FREE(&opstack);
	VEC_PUSH(&out, NULL);
	
	if(rpn) *rpn = out.data;
	if(rpnlen) *rpnlen = out.len;
	
	return 0;
	
	
PAREN_MISMATCH:
	VEC_FREE(&opstack);
	VEC_FREE(&out);
	
	if(rpn) *rpn = NULL;
	if(rpnlen) *rpnlen = 0;
	
	return STI_ERR_PAREN_MISMATCH;
}



// the thinnest of checks
static int string_is_number(char* s) {
	if(s[0] >= '0' && s[0] <= '9') return 1;
	
	else if(s[0] == '-' || s[0] == '+') { 
		if(s[1] >= '0' && s[1] <= '9') return 1;
		else if(s[1] == '.') {
			if(s[2] >= '0' && s[2] <= '9') return 1;
		}
	}
	else if(s[0] == '.') {
		if(s[1] >= '0' && s[1] <= '9') return 1;
	}
	
	return 0;
}



#define oper(Z) { \
	VEC_POP(&stack, b); \
	VEC_POP(&stack, a); \
	Z; \
	VEC_PUSH(&stack, c); \
	break; }

int64_t rpn_eval_int_str(char** rpn) {
	int64_t a, b, c;
	
	VEC(int64_t) stack;
	VEC_INIT(&stack);
	
	for(char** r = rpn; *r; r++) {
		if(string_is_number(*r)) {
			VEC_PUSH(&stack, strtol(*r, NULL, 10));
			continue;
		}
		
		switch(**r) {
			case '+': oper(c = a + b);
			case '-': oper(c = a - b);
			case '/': oper(c = a / b);
			case '%': oper(c = a % b);
			case '*': 
				if((*r)[1] == '*') oper(c = a; for(int64_t n = labs(b); n > 1; n--) c *= a)
				else oper(c = a * b);
			case '&': oper(c = a & b);
			case '|': oper(c = a | b);
			case '^': oper(c = a ^ b);
			case '<': 
				if((*r)[1] == '<') oper(c = a << b);
				break;
			case '>': 
				if((*r)[1] == '>') oper(c = a << b);
				break;
		}
	}
	
	c = VEC_ITEM(&stack, 0);
	
	VEC_FREE(&stack);
	
	return c;
}


double rpn_eval_double_str(char** rpn) {
	double a, b, c;
	
	VEC(double) stack;
	VEC_INIT(&stack);
	
	for(char** r = rpn; *r; r++) {
		if(string_is_number(*r)) {
			VEC_PUSH(&stack, strtod(*r, NULL));
			continue;
		}
		
		switch(**r) {
			case '+': oper(c = a + b);
			case '-': oper(c = a - b);
			case '/': oper(c = a / b);
			case '*': oper(c = a * b);
			case '%': oper(c = fmod(a, b));
		}
	}
	
	c = VEC_ITEM(&stack, 0);
	
	VEC_FREE(&stack);
	
	return c;
}



/*
int infix_to_rpn_incr(sti_op_prec_rule* rules, sti_shunting_context* ctx) {
	
	VEC_INIT(&ctx->stack);
	
	
}
*/

