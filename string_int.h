#ifndef __sti__string_int_h__
#define __sti__string_int_h__






struct string_internment_table;
typedef struct string_internment_table string_internment_table_t;


extern string_internment_table_t* global_string_internment_table;


// returns a pointer to the permanent unique string
char* strint_(string_internment_table_t* tab, char* s);
#define strint(a) strint_(global_string_internment_table, (a))

// returns a pointer to the permanent unique string
char* strnint_(string_internment_table_t* tab, char* s, size_t slen);
#define strnint(a, b) strnint_(global_string_internment_table, (a), (b))

// allocates and initializes memory for the table.
void string_internment_table_init(string_internment_table_t** ptab);

// frees all memory related to the table, including all the strings held inside
void string_internment_table_destroy(string_internment_table_t* tab);





#endif // __sti__string_int_h__
