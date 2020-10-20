# sti
General extension library for C. No dependencies beyond common POSIX 
platform headers. "[st]andard library, [i]zzy", in the same vein as stb.

`#include "sti/sti.h"` and compile `sti/sti.c` to get everything at once, or 
select individual pairs a la carte.


(Un)licensed under the Unlicense, which is effectively lawyer speak for 
Public Domain in all sane countries that have the concept of Public Domain. 
To quote Sam Hocevar, You just DO WHAT THE FUCK YOU WANT TO.

# Warning
The API may change drastically with no notice. This is my personal set of utilities and I have not
permanently settled on how it should work.

A few of the functions have terrible performance when compiled with -O0 but excellent performance
when compiled with -O2/3.

Some of the _EACH() loop macros use typeof, a gcc-specific extension. I am not currently aware of
a workaround for clang.  

# Features
* Auto-sizing array/vector<>/"stretchy buffer"
* Open-addressing hash table
* Red-black tree
* Sets
* Min/max heap
* Custom memory allocators
	* Memory pool using non-reserved virtual memory maps
	* Arena allocator using non-reserved virtual memory maps
	* talloc (tree allocator; simple wrapper around malloc)
* S-expression parser
* Generic, configurable shunting-yard algorithm to RPN
* Filesystem utlities:
	* List files in directory
	* Walk directories recursively with callback
	* Path manipulation functions
* String helpers
	* UTF-8/32 versions of [most] of the C std lib.
	* String splitting
* Basic statitistics calculation
* Miscellaneous math helpers
* Miscellaneous Public Domain hash functions
* Configurable state-machine based lexer generator

Most of the data structures use macro magic to be relatively type-safe and avoid void* casting 
where possible. Acknowledgements to nothings/Sean Barrett for the concept.

## TODO
* Documentation
* sexp could use some love:
	* Escaped chars string support
	* More api helper fns
	* Named arguments
* Basic tests for important fns
