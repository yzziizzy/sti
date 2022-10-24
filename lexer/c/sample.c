/*
#define bar(x) BAR
#ifdef bar
#define wrap(x) {x()}
#else
#define wrap(x) <x()>
#endif
#define foo bar

// #define X(a,b,c, ...) a.b->c; __VA_CNT__ + __VA_OPT__ (__VA_CNT__ X __VA_ARGS__) [__VA_ARGS__];
#define X(a,b,c, ...) a b #c

X(A, wrap(foo), C, DD, EE FF)
*/

#define ELEVEN 11

#if -(ELEVEN-2) == -19 % 10
	TRUE
#else
	FALSE
#endif


#define FOO_LIST \
	X(int, x) \
	X(float, y)
	
	
struct Foo {
	#define X(a, b) a* b;
	FOO_LIST
	#undef X
};

X(1,2)

#include "lexer.h"
//#include <cpp.h>



