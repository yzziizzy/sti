#ifndef __sti_rb_h__included__
#define __sti_rb_h__included__





typedef struct rb_node {
	struct rb_node* parent;
	char* key;
	void* data;
	struct rb_node* kids[2];
} rb_node;


typedef struct rb_tree_ {
	rb_node* root;
	size_t size;
} rb_tree_;


#define RB(t) \
union { \
	t** type; \
	rb_tree_ tree; \
}

#define RB_init(t) \
do { \
	(t)->tree.root = NULL; \
	(t)->tree.size = 0; \
} while(0);

#define RB_insert(t, k, v) rb_insert_(&(t)->tree, k, (void*)(intptr_t)(1 ? v : **((t)->type)))
void rb_insert_(rb_tree_* tree, char* key, void* val);

#define RB_delete(t, k, vp) rb_delete_(&(t)->tree, k, (void**)(1 ? vp : *((t)->type)))
int rb_delete_(rb_tree_* tree, char* key, void** data);

#define RB_find(t, k, f) (1 ? rb_find_(&(t)->tree, k, f) : (t)->type)
void* rb_find_(rb_tree_* tree, char* key, int* found);

#define RB_trunc(t) rb_trunc_(&(t)->tree)
void rb_trunc_(rb_tree_* t);


typedef long (*rb_traverse_fn)(char* key, void* value, void* user_data);

#define RB_traverse(t, f, u) rb_traverse_(&(t)->tree, f, u)
long rb_traverse_(rb_tree_* tree, rb_traverse_fn fn, void* user_data);
#define RB_r_traverse_(t, f, u) rb_r_traverse_(&(t)->tree, f, u)
long rb_r_traverse_(rb_tree_* tree, rb_traverse_fn fn, void* user_data);







// debugging

#ifdef DEBUG_RED_BLACK_TREE

void html_header(char* file_path); // also opens the file
void html_print_node(rb_node* n, int h); 
void html_spacer();
void html_footer(); // also closes the file
int get_max_height(rb_node* n);

FILE* dbg;
#endif // DEBUG_RED_BLACK_TREE

#endif // __sti_rb_h__included__
