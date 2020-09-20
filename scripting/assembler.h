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
	IT(label, 1, "") \
	IT(goto, 1, "") \
	IT(call, -1, "") \
	IT(ret, 0, "") \
	IT(cond, 3, "") \
	\
	IT(frame, 0, "start a new stack frame") \
	IT(unframe, 0, "roll back to the previous stack frame") \
	IT(args, -1, "push a set arguments") \
	IT(unargs, 1, "pop one set of arguments") \
	IT(local, 3, "reserve stack space for a local variable") \
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
	VT_sint,
	VT_uint,
	VT_f16, // IEEE754 half-precision float
	VT_f32,
	VT_f64,
	VT_fb16, // "binary 16", truncated f32
};



typedef struct Inst {
	int opid;
	int argc;
	char** args;
} Inst;

typedef struct Ilabel {
	char* name;
} Ilabel;

typedef struct Ivar {
	enum VarType type;
	int width;
} Ivar;


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

typedef struct Context {
// 	HashTable(tivar*) heap;
	
	Inst* inst;
	size_t instAlloc;
	size_t instLen;
	
	HashTable(size_t) labels;
	
	char* stack;
	size_t stackAlloc;
	size_t stackBase;
	size_t stackHead;
	
	VEC(FrameInfo) frames;
	
	size_t ip;
	char halt;
	
} Context;











#endif // __sti_scripting_assembler_h__
