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
/*
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
*/


X(1,2)

#define __A 123
#define __B 345

# if __A == __B
	false
# else
	true
# endif



/*
#if 1 == 1
	yes 1
		
	#if 1
		yes 2
		
		#if 0
			no 1
			#if 1
				no 2
				#if 1 
					no 3
				#else
					no 4
				#endif
			#else
				no 5
			#endif
		#else
			yes 3
		#endif	
	#else
		no 6
	#endif
	yes 4 final
#else
	no 7
#endif	

foo
*/
//#include "lexer.h"
//#include <bits/wctype-wchar.h>



