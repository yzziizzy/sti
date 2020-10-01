


#include "../../../sti.h"

enum LexState {
	LST_INVALID,
	#define PARSER_INCLUDE_ENUMS
	#include "./parser_example_generated.h"
	#undef PARSER_INCLUDE_ENUMS
	LST_MAXVALUE
};

extern char* state_names[];


typedef struct LexerToken {
	int tokenState;
	
	int line;
	int character;
	char* sourceFile;
	char* tokenText;
} LexerToken;

typedef struct TokenStream {
	VEC(LexerToken) tokens;
	
	VEC(char*) filePathCache;
} TokenStream;

TokenStream* LoadAndLexFile(char* path);
