

// single file to build everything


#include "talloc.c"
#include "heap.c"
#include "fs.c"
#include "hash.c"
#include "hash_fns/MurmurHash3.c"
#include "hash_fns/sha.c"
#include "misc.c"
#include "sets.c"
#include "sexp.c"
#include "slot.c"
#include "string.c"
#include "utf.c"
#include "vec.c"
#include "rb.c"
#include "rpn.c"
#include "ring.c"
#include "ini.c"
#include "string_int.c"
#include "matrix.c"
#include "b64.c"
//#include "sort.c"

// semi-platform-dependent
//#include "objdump.c"

// heavily dependent on Linux
#include "memarena.c"
#include "mempool.c"



