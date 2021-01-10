#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/mman.h>


#include "../vec.h"
#include "../hash.h"
#include "../rpn.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


typedef  unsigned char byte;






size_t parse(char* input, char*** output) {
	char* s = input;
	size_t alloc = 32;
	size_t len = 0;
	char** out = malloc(alloc * sizeof(*out));
	
	
	for(; *s; ) {
		if(len >= alloc) {
			alloc *= 2;
			out = realloc(out, alloc * sizeof(*out));
		}
		
		if(isalnum(*s) || *s == '.') {
			char* e = s;
			while(*e && (isalnum(*e) || *e == '.')) e++;
			
			out[len] = strndup(s, e - s);
			len++;
			
			s = e;
			continue;
		}
		
		switch(*s) {
			case '*':
			case '/':
			case '+':
			case '-':
			case '(':
			case ')':
			case '[':
			case ']':
			case ',':
				out[len] = strndup(s, 1);
				len++;
				s++;
				break;
				
			case ' ':
			case '\r':
			case '\n':
				s++;
				continue;
		}
	}
	
	// null-terminate the list
	if(len >= alloc) {
		out = realloc(out, (alloc+1) * sizeof(*out));
	}
	out[len] = NULL;
		
	*output = out;
	
	return len;
}


#define INST_TYPES \
	X(MOV) \
	X(LOAD) \
	X(STORE) \
	X(ADD) \
	X(SUB) \
	X(MUL) \
	X(DIV) \


enum InstructionType {
	#define X(x) INST_##x,
		INST_TYPES
	#undef X
};

char* InstructionTypeNames[] = {
	#define X(x) [INST_##x] = #x,
		INST_TYPES
	#undef X
};





// W = 1 means 64 bit operand size
// R is the MSB of the modr/m reg field
// X is the MSB of the sib index field
// B is the MSB of the modr/m r/m field, sib base field, or opcode reg field
byte encodeREX(char W, char R, char X, char B) {
	return 0 | (1 << 6) | (!!W << 3) | (!!R << 2) | (!!X << 1) | (!!B);
}

// reg usually encodes the second operand, usually the destination
// mod+r/m usually encode the first operand
// reg sometimes stores extra opcode information
// rarely, mod+rm can store extra opcode info

byte encodeModRM(char mod, char reg, char rm) {
	byte out = 0;
	
	out |= mod << 6;
	out |= (reg & 0x3) << 3;
	out |= (rm & 0x3);
	
	return out;
}

// mod = 11, r/m = 0..8
//   AX, CX, DX, BX, SP, BP, SI, DI


// In 64-bit mode, the default address size is 64 and operand size is 32

// legacy prefix, rex prefix, opcode, modr/m, sib, disp, imm
// rex prefix comes after mandatory prefix and before escape bytes


// MOVUPS  0f 10   reg = dst, r/m = src
// MOVUPS  0f 11   reg = src, r/m = dst
// MOVAPS  0f 28   reg = dst, r/m = src
// MOVAPS  0f 29   reg = src, r/m = dst
// ADDPS   0f 58   reg = dst, r/m = src
// MULPS   0f 59   reg = dst, r/m = src
// SUBPS   0f 5C   reg = dst, r/m = src
// DIVPS   0f 5E   reg = dst, r/m = src
// MINPS   0f 5d   reg = dst, r/m = src
// MAXPS   0f 5f   reg = dst, r/m = src
// SQRTPS  0f 51   reg = dst, r/m = src  
// RSQRTPS 0f 52   reg = dst, r/m = src  
// RCPPS   0f 53   reg = dst, r/m = src  // reciprocal


typedef struct AssemblerInfo {
	int type;
	unsigned char opcodes[3];
	char numOpcodes;
	char dstIsRM;
	char* textName;
} AssemblerInfo;


AssemblerInfo assemblerLookup[] = {
	{INST_LOAD,  {0x0f, 0x10}, 2, 0, "movups"},
	{INST_STORE, {0x0f, 0x11}, 2, 1, "movups"},
	{INST_MOV, {0x0f, 0x28}, 2, 0, "movaps"},
	{INST_ADD, {0x0f, 0x58}, 2, 0, "addps"},
	{INST_MUL, {0x0f, 0x59}, 2, 0, "mulps"},
	{INST_SUB, {0x0f, 0x5c}, 2, 0, "subps"},
	{INST_DIV, {0x0f, 0x5e}, 2, 0, "divps"},
};

AssemblerInfo* findInstr(enum InstructionType type) {
	for(int i = 0; i < sizeof(assemblerLookup) / sizeof(assemblerLookup[0]); i++) {
		AssemblerInfo* a = &assemblerLookup[i];
		if(type == a->type) return a;
	}
	
	return NULL;
}



#define TOKEN_TYPES \
	X(NONE) \
	\
	X(ADD) \
	X(SUB) \
	X(MUL) \
	X(DIV) \
	X(MIN) \
	X(MAX) \
	X(SQRT) \
	X(RECIP) \
	\
	X(IMMED) \
	X(CONST) \
	X(VAR) \
	
	
enum TokType {
	#define X(x) T_##x,
		TOKEN_TYPES
	#undef X
};

char* TokTypeNames[] = {
	#define X(x) [T_##x] = #x,
		TOKEN_TYPES
	#undef X
};


enum TokType classifyToken(char* s) {
	switch(*s) {
		case '+': return T_ADD;
		case '-': return T_SUB;
		case '*': return T_MUL;
		case '/': return T_DIV;
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
			return T_CONST;
		
		default: return T_VAR;
	}
}



typedef struct Node {
	char* text;
	enum TokType type;
	struct Node* l, *r;
	
	int resultVar;
	
} Node;
	

void printNode(Node* n, int indent) {
	for(int i = 0; i < indent; i++) printf(" ");
	
	printf("%s (%d): ", n->text, n->resultVar);
	if(n->l) printf("%s", n->l->text);
	else printf("_");
	
	printf(" / ");
	
	if(n->r) printf("%s", n->r->text);
	else printf("_");
	
	printf("\n");

	if(n->l) printNode(n->l, indent + 2);
	if(n->r) printNode(n->r, indent + 2);
}

typedef struct RegAllocInfo {
	int nextVar;
} RegAllocInfo;


void assignRegisters(Node* n, RegAllocInfo* info) {
	if(n->l) assignRegisters(n->l, info);
	if(n->r) assignRegisters(n->r, info);
	
	n->resultVar = info->nextVar++;
}


typedef struct ThreeAddressInstr {
	char* text;
	enum TokType type;
	int out;
	int inL, inR;
} ThreeAddressInstr;

typedef struct VarDef {
	int varNum;

	int slot;
	char slotType; // 0 = temp/var, 1 = input, 2 = const
	float constVal;
	char* varTextName;
	int regNum;

	int assignmentInst;
	int lastUsedInst;

} VarDef;

typedef struct SlotInfo {
	char* textName;
	int varNum;
	size_t byteOffset;
	size_t elemSize;
} SlotInfo;

typedef struct AsmInst {
	int instID;
	short dstReg; // register num or slotType
	short srcReg;
	char dstType; // 0 = reg, 1 = mem
	char srcType; // 0 = reg, 1 = mem
	size_t memOffset;
	
	unsigned char* machineCode;
	int mcLen;
} AsmInst;




typedef struct LinearizationInfo {
	VEC(ThreeAddressInstr*) ilist;
	int varCnt;
	int inputSlotCnt;
	int constSlotCnt;
	int tempSlotCnt;
	
	VarDef* vars;
	SlotInfo* inputSlots;
	SlotInfo* constSlots;
	SlotInfo* tempSlots;
	
	VEC(int) freeRegStack;
	
	VEC(AsmInst) code;
	
} LinearizationInfo;




void encodeInst(AsmInst* inst) {
	byte temp[16]; // x64 instructions cannot be longer than 15 bytes, per the ISA
	int len = 0;
	byte modrm;
	byte disp8;
	unsigned short disp16;
	char hasDisp8 = 0;
	char hasDisp16 = 0;
	
	AssemblerInfo* a = findInstr(inst->instID);
	
	// TODO: prefixes here
	
	// opcode bytes
	for(int i = 0; i < a->numOpcodes; i++) {
		temp[len++] = a->opcodes[i];
	}
	
	// modr/m byte
	
	if(inst->srcType == 1) {
		// memory
		if(a->dstIsRM) {
//			modrm = encodeModRM(0x3, inst->srcReg, inst->dstReg);
			printf("\n--STORE assembly not implemented--\n\n");
		}
		else {
			modrm = encodeModRM(0x2, inst->dstReg, 0x5); // [rdi]+disp16
			hasDisp16 = 1;
			disp16 = inst->memOffset;
		}
	}
	else if(inst->srcType == 0 && inst->dstType == 0) {
		// for registers
		// mod = 11b, r/m = 0..7  (xmm0..xmm7)
		//   AX, CX, DX, BX, SP, BP, SI, DI
		if(a->dstIsRM) {
			modrm = encodeModRM(0x3, inst->srcReg, inst->dstReg);
		}
		else {
			modrm = encodeModRM(0x3, inst->dstReg, inst->srcReg);
		}
	}
	
	
	temp[len++] = modrm;
	
	if(hasDisp8) temp[len++] = disp8;
	else if(hasDisp16) {
		temp[len++] = disp16 & 0xff;
		temp[len++] = disp16 >> 8;
	}
	
	inst->machineCode = malloc(len);
	memcpy(inst->machineCode, temp, len);
	inst->mcLen = len;
}

void assembleCode(LinearizationInfo* info) {
	VEC_EACHP(&info->code, i, inst) {
		encodeInst(inst);
	}
}




void generateCode(LinearizationInfo* info) {
	
	#define REG 0
	#define MEM 1
	#define TEMP 0
	#define INPUT 1
	#define CONST 2
	
	#define INST(id, dtype, dreg, stype, sreg, moff) \
		VEC_PUSH(&info->code, ((AsmInst){ \
			.instID = INST_##id, \
			.dstType = dtype, \
			.srcType = stype, \
			.dstReg = dreg,  \
			.srcReg = sreg, \
			.memOffset = moff, \
		}));
	
	VarDef* o, *l, *r;
	VEC_EACH(&info->ilist, i, tia) {
		switch(tia->type) {
			case T_CONST:
				// load from const mem area
				o = &info->vars[tia->out];
				INST(MOV, REG, o->regNum, MEM, CONST, info->constSlots[info->vars[tia->out].slot].byteOffset);
				break;
				
			case T_VAR:
				printf("T_VAR not supported.\n");
				break;
			
			case T_ADD:
				o = &info->vars[tia->out];
				l = &info->vars[tia->inL];
				r = &info->vars[tia->inR];
				INST(MOV, REG, o->regNum, REG, l->regNum, 0);
				INST(ADD, REG, o->regNum, REG, r->regNum, 0);
				break;
					
			case T_SUB:
				o = &info->vars[tia->out];
				l = &info->vars[tia->inL];
				r = &info->vars[tia->inR];
				INST(MOV, REG, o->regNum, REG, l->regNum, 0);
				INST(SUB, REG, o->regNum, REG, r->regNum, 0);
				break;
					
			case T_MUL:
				o = &info->vars[tia->out];
				l = &info->vars[tia->inL];
				r = &info->vars[tia->inR];
				INST(MOV, REG, o->regNum, REG, l->regNum, 0);
				INST(MUL, REG, o->regNum, REG, r->regNum, 0);
				break;
					
			case T_DIV:
				o = &info->vars[tia->out];
				l = &info->vars[tia->inL];
				r = &info->vars[tia->inR];
				INST(MOV, REG, o->regNum, REG, l->regNum, 0);
				INST(DIV, REG, o->regNum, REG, r->regNum, 0);
				break;	
			
		}
		
	}
	
}





void setupSlots(LinearizationInfo* info) {
	
	info->inputSlots = calloc(1, info->inputSlotCnt * sizeof(*info->inputSlots));
	info->constSlots = calloc(1, info->constSlotCnt * sizeof(*info->constSlots));
	info->tempSlots = calloc(1, info->tempSlotCnt * sizeof(*info->tempSlots));
	
	for(int i = 1; i < info->varCnt; i++) {
		VarDef* v = &info->vars[i];
		
		if(v->slotType == 0) {
			SlotInfo* slot = &info->tempSlots[v->slot];
			slot->varNum = v->varNum;
			slot->elemSize = 4;
			slot->byteOffset = 4 * v->slot;
		}
		else if(v->slotType == 1) {
			SlotInfo* slot = &info->inputSlots[v->slot];
			slot->varNum = v->varNum;
			slot->textName = v->varTextName;
			slot->elemSize = 4;
			slot->byteOffset = 4 * v->slot;
		}
		else if(v->slotType == 2) {
			SlotInfo* slot = &info->constSlots[v->slot];
			slot->varNum = v->varNum;
			slot->elemSize = 4;
			slot->byteOffset = 4 * v->slot;
		}
		
		
		if(VEC_LEN(&info->freeRegStack) > 0) {
			VEC_POP(&info->freeRegStack, v->regNum);
		}
		
		
		
	}
	
	
	
	
	
}


void linearizeAST(Node* n, LinearizationInfo* info) {
	ThreeAddressInstr* tai;

	if(n->l) linearizeAST(n->l, info);
	if(n->r) linearizeAST(n->r, info);
	
	int instIndex = VEC_LEN(&info->ilist);
	
	VarDef* var = &info->vars[n->resultVar];
	var->varNum = n->resultVar;
	if(n->type == T_VAR) {
		var->varTextName = n->text;
		var->slotType = 1;
		var->slot = info->inputSlotCnt++;
	}
	else if(n->type == T_CONST) {
		var->constVal = strtof(n->text, NULL);
		var->slotType = 2;
		var->slot = info->constSlotCnt++;
	}
	else {
		var->slotType = 0;
		var->slot = info->tempSlotCnt++;	
	}
	
	var->assignmentInst = instIndex;
	var->lastUsedInst = instIndex;
	
	
	
	tai = calloc(1, sizeof(*tai));
	tai->text = n->text;
	tai->type = n->type;
	tai->out = n->resultVar;
	if(n->l) {
		tai->inL = n->l->resultVar;
		info->vars[n->l->resultVar].lastUsedInst = MAX(instIndex, info->vars[n->l->resultVar].lastUsedInst);
	}
	if(n->r) {
		tai->inR = n->r->resultVar;
		info->vars[n->r->resultVar].lastUsedInst = MAX(instIndex, info->vars[n->r->resultVar].lastUsedInst);
	}
		
	VEC_PUSH(&info->ilist, tai);
	
}



void compile(char** source, size_t slen) {


	sti_op_prec_rule rules[] = {
		{"",   0, STI_OP_ASSOC_NONE,  0},
		{"+",  1, STI_OP_ASSOC_LEFT,  2},
		{"-",  1, STI_OP_ASSOC_LEFT,  2},
		{"*",  2, STI_OP_ASSOC_LEFT,  2},
//		{"**", 3, STI_OP_ASSOC_LEFT,  2},
		{"/",  2, STI_OP_ASSOC_LEFT,  2},
		{"(",  8, STI_OP_OPEN_PAREN,  0},
		{")",  8, STI_OP_CLOSE_PAREN, 0},
		{"[",  9, STI_OP_OPEN_PAREN,  0},
		{"]",  9, STI_OP_CLOSE_PAREN, 0},
		{NULL, 0, 0, 0},
	};



	char** rpn;
	size_t len;
	
	infix_to_rpn(rules, source, &rpn, &len);
	
	len--;
	
	printf("\n");
	for(int i = 0; i < len; i++) {
		printf("^%d - '%s'\n", i, rpn[i]);
	}
	
	typedef struct Token {
		enum TokType type;
		char* text;
		
	} Token;
	
	
	Token* toks = calloc(1, len * sizeof(*toks));
	for(int i = 0; i < len; i++) {
		toks[i].type = classifyToken(rpn[i]);
		toks[i].text = rpn[i];
	}
	
	
	
	
	VEC(Node*) stack;
	VEC_INIT(&stack);
	
	for(int i = 0; i < len; i++) {
		Node* n = calloc(1, sizeof(*n));
		n->text = toks[i].text;
		n->type = toks[i].type;
	
		if(toks[i].type == T_CONST || toks[i].type == T_VAR) {
			VEC_PUSH(&stack, n);
			
			continue;
		}
		
		// operators. all are binary right now
		VEC_POP(&stack, n->r);
		VEC_POP(&stack, n->l);
		VEC_PUSH(&stack, n);
	}
	
	Node* root;
	VEC_POP(&stack, root);
	
	// now in AST form
	
	RegAllocInfo reginfo;
	reginfo.nextVar = 1;
	
	assignRegisters(root, &reginfo);
	printNode(root, 0);
	
	// now in SSA
	
	LinearizationInfo linfo;
	memset(&linfo, 0, sizeof(linfo));
	linfo.varCnt = reginfo.nextVar;
	linfo.vars = calloc(1, (linfo.varCnt+1) * sizeof(*linfo.vars));
	
	for(int i = 1; i <= 7; i++) { 
		VEC_PUSH(&linfo.freeRegStack, i);
	}
	
	
	linearizeAST(root, &linfo);
	
	printf("\nInstructions:\n");
	VEC_EACH(&linfo.ilist, i, tai) {
		if(tai->type == T_IMMED) {
			printf("  %ld> %d = %s\n", i, tai->out, tai->text);
		}
		else
			printf("  %ld> %d = %d %s %d\n", i, tai->out, tai->inL, TokTypeNames[tai->type], tai->inR);
	}
	
	printf("\nVariables:\n");
	for(int i = 1; i < linfo.varCnt; i++) {
		VarDef* def = &linfo.vars[i];
		
		printf("  %d: first: %d, last: %d, ", def->varNum, def->assignmentInst, def->lastUsedInst);
		if(def->slotType == 0) {
			printf("tempvar slot = %d\n", def->slot);
		}
		else if(def->slotType == 1) {
			printf("input slot = %d, name = '%s'\n", def->slot, def->varTextName);
		}
		else if(def->slotType == 2) {
			printf("const slot = %d, value = %f\n", def->slot, def->constVal);
		}
	}
	
	// now in SSA, Three Address Code
	
	setupSlots(&linfo);
	
	// memory layout is chosen
	
	generateCode(&linfo);
	
	printf("\nInstructions:\n");
	VEC_EACHP(&linfo.code, i, inst) {
		printf("  %ld> %s ", i, InstructionTypeNames[inst->instID]);
		if(inst->dstType == 0) {
			printf("xmm%d, ", inst->dstReg); 
		}
		else if(inst->dstType == 1) {
			printf("[CONST+%d], ", inst->memOffset);
		}
		
		if(inst->srcType == 0) {
			printf("xmm%d\n", inst->srcReg); 
		}
		else if(inst->srcType == 1) {
			printf("[CONST+%d]\n", inst->memOffset);
		}
	}
	
	
	assembleCode(&linfo);
	
	// done
	
	printf("\nMachine Code:\n");
	VEC_EACHP(&linfo.code, i, inst) {
		printf("  %ld> ", i);
		for(int j = 0; j < inst->mcLen; j++) {
			printf("%.2x ", inst->machineCode[j]);
		}
		printf("\n");
	}
}


unsigned char prog[] = {
0x8d, 0x47, 0x04, 0xc3,
};


typedef int (*asmfn)(int);


int main(int argc, char* argv[]) {

	char** tokens;
	size_t len = parse("3.14*74.2-66+82.36", &tokens);
	
	for(int i = 0; i < len; i++) {
		printf("%d - '%s'\n", i, tokens[i]);
	}
	
	compile(tokens, len);
	
	return 0;
	
	asmfn fn = mmap(NULL, 64, PROT_EXEC|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	
	memcpy(fn, prog, 4);
	
	int b = fn(3);
	
	printf("b: %d\n", b);
	

	return 0;
}





