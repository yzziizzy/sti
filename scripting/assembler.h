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
	IT(local, 2, "reserve stack space for a local variable") \
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






#endif // __sti_scripting_assembler_h__
