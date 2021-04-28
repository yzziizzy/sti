#ifndef __arithmetic_h__
#define __arithmetic_h__



typedef  unsigned char byte;


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



// Argument passing:
// rdi, rsi, rdx, rcx, r8, r9, ....stack



typedef struct AssemblerInfo {
	int type;
	unsigned char opcodes[3];
	char numOpcodes;
	char dstIsRM;
	char* textName;
	
	// int alignment;
} AssemblerInfo;



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

// AST Node
typedef struct Node {
	char* text;
	enum TokType type;
	struct Node* l, *r;
	
	int resultVar;
	
} Node;


// context struct for the reg alloc algorithm
typedef struct RegAllocInfo {
	int nextVar;
} RegAllocInfo;



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


void linearizeAST(Node* n, LinearizationInfo* info);
void setupSlots(LinearizationInfo* info);
void assembleCode(LinearizationInfo* info);
void generateCode(LinearizationInfo* info);


typedef void (*arithmetic_fn)(float* /*in*/, float* /*out*/, float* /*constants*/);


typedef struct ProgramInfo {
	size_t mcLen;
	arithmetic_fn fn;
	
	int width; // SIMD width. 4 for now with SSE *ps instructions
	
	float* constants;

} ProgramInfo;



ProgramInfo* compile(char** source, size_t slen);



#endif // __arithmetic_h__
