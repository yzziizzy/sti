#ifndef __sti_scripting_assembler_h__
#define __sti_scripting_assembler_h__
// Public Domain

#include "../sti.h"


/*

jump tables for switch()
type conversions, swizzles, unpacks
*/

// assembly instructions
//     name, num args
#define IT_CROWD \
	IT(debug, 1, "") \
	IT(stack_dump, 1, "") \
	\
	IT(halt, 0, "") \
	IT(label, -1, "") \
	IT(goto, -1, "") \
	IT(call, -1, "") \
	IT(tail, -1, "tail-call another compatible function") \
	IT(resume, -1, "Goes after a call and defines the arguments returned") \
	IT(ret, 0, "") \
	IT(cond, 3, "") \
	\
	IT(func, -1, "") \
	IT(args, -1, "") \
	IT(returns, -1, "") \
	IT(local, 3, "reserve stack space for a local variable") \
	IT(disfunc, 0, "mark the absolute end of the current function") \
	\
	IT(set, 2, "set a local variable's value") \
	\
	IT(add, 3, "") \
	IT(sub, 3, "") \



// get stack frame size, current total stack size

enum InstType {
	IT_NONE,
	
#define IT(n, x, y) IT_##n,
	IT_CROWD
#undef IT

	IT_MAX_VALUE,
};

static int InstArgCounts[] = {
	#define IT(n, x, y) [IT_##n] = x,
		IT_CROWD
	#undef IT
};

enum VarType {
	VT_INVALID,
	VT_s8,
	VT_s16,
	VT_s32,
	VT_s64,
	VT_u8,
	VT_u16,
	VT_u32,
	VT_u64,
// 	VT_f16, // IEEE754 half-precision float
	VT_f32,
	VT_f64,
	VT_buffer,
	VT_utf8,
// 	VT_fb16, // "binary 16", truncated f32
};



typedef struct Inst {
	int opid;
	int argc;
	char** args;
} Inst;


typedef struct Ivar {
	enum VarType type;
	char* structName;
	int width;
} Ivar;


typedef struct Ilabel {
	char* name;
	int varc;
	Ivar* vars;
} Ilabel;


typedef struct StructMember {
	char* name;
	int ordinal;
	int offset;
	Ivar var;
} StructMember;

typedef struct StructDef {
	char* name;
	int membc;
	StructMember* members;
} StructDef;

typedef struct Label {
	char* name;
	size_t location;
} Label;

typedef struct Assembler {
	
	int foo;
	
} Assembler;



enum LexState {
	LST_INVALID,
	
	#include "./lexer_enums.h"
	
	LST_MAXVALUE
};



static char* lexer_state_names[] = {
	[LST_INVALID] = "LST_INVALID",
	#include "./lexer_enum_names.h"
	[LST_MAXVALUE] = "LST_MAXVALUE",
};

typedef struct Lexer {
	enum LexState state;
	char* buffer;
	int blen;
	int balloc;
	
	int linenum;
	int charnum;
	
	enum LexState tokenState;
	int tokenFinished; // buffer should be consumed and cleaned at this point 
	
	char* source;
	size_t sourceLen;
	int i;
} Lexer;



Lexer* start_lexer(char* source, size_t len);
int next_token(Lexer* ls);






typedef struct LocalInfo {
	char* name;
	enum VarType type;
	int width;
	
	size_t offset;
} LocalInfo;


typedef struct FrameInfo {
// 	HashTable(size_t) labels;
	HashTable(LocalInfo*) locals;
} FrameInfo;





typedef struct ArgInfo {
	char* name;
	enum VarType type;
	int width;
	int ordinal;
} ArgInfo;

typedef VEC(ArgInfo) ArgList;
typedef VEC(Inst) InstList;


typedef struct FunctionInfo {
	char* name;
	
	InstList inst;
	
	ArgList args;
	ArgList rets;
	
	HashTable(size_t) labels;
	HashTable(LocalInfo*) locals;
} FunctionInfo;

typedef struct FunctionCtx {
	HashTable(LocalInfo*) locals;
} FunctionCtx;

typedef struct Context {
// 	HashTable(tivar*) heap;
	
	/*
	Inst* inst;
	size_t instAlloc;
	size_t instLen;
	*/
	
	HashTable(FunctionInfo*) functions;
	
	char* stack;
	size_t stackAlloc;
	size_t stackBase;
	size_t stackHead;
	
	VEC(FrameInfo) frames;
	
	size_t ip;
	char halt;
	
} Context;











#endif // __sti_scripting_assembler_h__
