
/*
indent with tabs up to the indent level, then spaces for visual alignment of code broken lines.

int foo() {
<tab>if(rand() < 10) {
<tab><tab>if((some_really_long_var != some_really_long_expression) ||
<tab><tab>   (more_really_long_stuff == last_really_long_stuff)) {
          ^^^ spaces
<tab><tab><tab>return 1;
<tab><tab>}
<tab>}
<tab>   <--- empty lines also indented
<tab>return 0;
}

Note how the above if statement is broken onto a separate line. Avoid spreading the code too thin, like below:
if(
	(some_really_long_var != some_really_long_expression) ||
	(more_really_long_stuff == last_really_long_stuff)
) {

This is just ugly and hard to read. Two entire lines are wasted for the if( and the ) {, to no real benefit
  and in conflict with the general nature of the rest of the formatting. Judgement is key; the ugly version
  may be best for an if statement that spans 8 lines, but not for two or three.



int* foo; <-- C++ style asterisk placement (with the type)
int* foo, *bar;  <--- multiple pointers on same line
int***** foo;


Never mix pointer and non-pointer variables in the same declaration:
int* foo, bar; <--- bad
int foo, *bar; <--- bad

int* foo;  \___ good
int bar;   /

typedef struct node { <--- typedefs and opening braces go on the same line
                  ^------ space before opening brace
	float x, y, z;
	union {
		unsigned long id;
		char* name;
	};
	     <--------------- blank line for general clarity, separating the list pointers from the data
	struct node* next, *prev;
} node_t; <--- closing brace at original indent, typedef'd name on same line
      ^^------ type name has _t suffix

for(int i = 0, j = 1; i < (j + 1); i++) {
  ^^         ^^        ^  ^^    ^^^ ^  ^ space before opening braces
  |          |         |  |     |   +--- x++ preferred to ++x where semantically equivalent
  |          |         |  |     +------- in for, no space before ; but yes space after unless the next token is also ; (see below) 
  |          |         |  +------------- no spaces after opening parens or before closing parens
  |          |         +---------------- spaces around operators
  |          +-------------------------- space after , but not before
  +------------------------------------- no space between keywords and their opening paren


for formatting:

for(;;) { <-- never use; write while(1) instead
for(int i = 0;;) {
for(;x < 3;) {  <-- never use; write while(x < 3) instead
for(; i < 3; i++) {


if(x < 10) {  <-------- opening braces are always on the same line as the closing paren of their condition
	printf("single");
}  <------------------- closing braces are at the same indent level as the original keyword 
else if(x < 100) {  <-- chained clauses are on the next line and the same indent level as the first
	printf("tens");
}
else {
	printf("big");
}


static const char* strstr(const char* haystack, const char* needle) { <--- opening brace on same line                                                            
^^^^^^^^^^^^^^^^^^      ^^--------------------^^-----------------^^^------ note space conventions
       +------------------------------------------------------------------ return values and all modifiers on the same line

	// function contents
} <-- closing brace at original indent


char* s = strstr(x, "foo");



float excessively_long_arg = (  <----- don't nest excessively long computations inside a function call. move to a var beforehand
	<multiple lines of calculations>
);

int ret = complicated_function( <------ opening paren on the same line, also the return value assignment
	arg1,   <-------------------------- arguments each on their own line, even if they are short 
	really_really_long_arg2[with.some[other]->stuff], 
	arg3_x, arg3_y,         \__________ logically grouped arguments can be on the same line if they are short enough
	arg5_x, arg5_y, arg5_z, /
	excessively_long_arg  <------------ see above
);  <---------------------------------- closing paren on its own line at original indent level 

Declarations of functions with many arguments follows a similar pattern:
int many_args_fn(
	<args>
) {
	<code>
	return 0;
}	

Functions with many arguments should be avoided when reasonable. Many arguments is often a sign of bad design, structuring, or factoring. 

Everything should compile with no warnings on -std=std11 -Werror -Wall -Wextra -Wpedandic 

*/


