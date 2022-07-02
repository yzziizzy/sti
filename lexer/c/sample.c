
#define bar(x) BAR
#define wrap(x) {x()}
#define foo bar

// #define X(a,b,c, ...) a.b->c; __VA_CNT__ + __VA_OPT__ (__VA_CNT__ X __VA_ARGS__) [__VA_ARGS__];
#define X(a,b,c, ...) a ## b #c

X(A, wrap(foo), C, DD, EE FF)






