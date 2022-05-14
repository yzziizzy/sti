#ifndef __sti__string_int_h__
#define __sti__string_int_h__






struct string_internment_table;


extern struct string_internment_table global_string_internment_table;


// returns a pointer to the permanent unique string
char* strint_(struct string_internment_table* tab, char* s);
#define strint(a) strint_(&global_string_internment_table, (a))

// returns a pointer to the permanent unique string
char* strnint_(struct string_internment_table* tab, char* s, size_t slen);
#define strnint(a, b) strnint_(&global_string_internment_table, (a), (b))

void string_internment_table_init(struct string_internment_table* tab);





#endif // __sti__string_int_h__
