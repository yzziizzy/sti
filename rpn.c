// Public Domain

#include "rpn.h"
#include "vec.h"



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
		
		while(VEC_LEN(&opstack) > 0) {
			sti_op_prec_rule top = VEC_TAIL(&opstack);
			
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
	
	*rpn = out.data;
	*rpnlen = out.len;
	
	return 0;
}


