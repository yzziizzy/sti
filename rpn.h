#ifndef __sti__rpn_h__
#define __sti__rpn_h__


enum {
	STI_OP_ASSOC_LEFT = -1,
	STI_OP_ASSOC_NONE = 0,
	STI_OP_ASSOC_RIGHT = 1,
	STI_OP_OPEN_PAREN = 2,
	STI_OP_CLOSE_PAREN = 3,
};


typedef struct sti_op_prec_rule {
	char* token;
	short prec;
	char assoc;
	char arity;
} sti_op_prec_rule;


int infix_to_rpn(sti_op_prec_rule* rules, char** infix, char*** rpn, size_t* rpnlen);



#endif // __sti__rpn_h__
