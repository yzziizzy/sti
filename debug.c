
#include "debug.h"
#include <stdio.h>


sti_DebugTypeInfo sti_debug_type_info[STI_DBG_TYPE_MAX_VALUE] = {
#define X(a, b,c,d, e,f,g, h, ...) [STI_DBG_TYPE_##a] = {#a, b,c,d, e,f,g, h},
	STI_DEBUG_TYPE_LIST(X)
#undef X
};



void sti_debug_print_var(char* filename, long linenum, const char* funcname, char* name, void* var, int type) {

	printf("%s:%ld %s = ", strstr(filename, "src/") + 4, linenum, name);
	
	sti_DebugTypeInfo* ti = &sti_debug_type_info[type];
	
	if(ti->ind_level > 0) {
		var = *(void**)var;
	}
	
	for(int i = 0; i < ti->vec_len; i++) { 
		if(i > 0) printf(", ");
		if(ti->is_float) {
			if(ti->size == 4) printf("%f", ((float*)var)[i]);
			else printf("%f", *(double*)var);
		}
		else if(ti->is_int) {
			if(ti->size == 1)      printf("%hhd", ((int8_t*)var)[i]);
			else if(ti->size == 2) printf("%hd", ((int16_t*)var)[i]);
			else if(ti->size == 4) printf("%d",  ((int32_t*)var)[i]);
			else if(ti->size == 8) printf("%ld", ((int64_t*)var)[i]);
		}
		else if(ti->is_string) {
			printf("%s", *(char**)var);
		}
	}
	
	printf("\n");
}




