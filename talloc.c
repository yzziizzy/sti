#include <stdlib.h>

#include "talloc.h"



struct trec {
	struct trec* parent;
	struct trec* child;
	struct trec* sibling;
};




void* talloc(void* parent, size_t size) {
	struct trec* c = malloc(size + sizeof(struct trec));
	if(parent) {
		struct trec* p = &((struct trec*)parent)[-1];
		c->sibling = p->child;
		p->child = c;
		c->parent = p;
	}
	else {
		c->sibling = NULL;
		c->parent = NULL;
	}
	
	c->child = NULL;
	
	return &c[1];
}


void* trealloc(void* old, size_t size) {
	struct trec* o = &((struct trec*)old)[-1];
	struct trec* n; 
	
	n = realloc(o, size + sizeof(struct trec));
	if(n == o) return n;
	
	// fix parent
	struct trec* p = n->parent;
	if(p->child == o) {
		p->child = n;
	}
	else {
		struct trec* z = p->child;
		while(z->sibling != o) z = z->sibling;
		z->sibling = n;
	}
	
	// fix children
	struct trec* z = n->child;
	while(z) {
		z->parent = n;
		z = z->sibling;
	}
	
	return n;
}



// designed for cascading frees, no parent list fixing
static void tfree_fast(struct trec* p) {
	struct trec* c = p->child;
	struct trec* o;
	
	// free all children
	while(c) {
		o = c;
		c = c->sibling;
		tfree_fast(o);
	}
	
	free(p);
}

void tfree(void* m) {
	// parent is being deleted
	struct trec* p = &((struct trec*)m)[-1];
	struct trec* c = p->child;
	struct trec* o;
	
	// free all children
	while(c) {
		o = c;
		c = c->sibling;
		tfree_fast(o);
	}
	
	// fix the grandparent's linked list
	struct trec* g = p->parent;
	if(g) {
		struct trec* n = g->child;
		if(n == p) {
			g->child = p->sibling;
		}
		else {
			while(n->sibling != p) n = n->sibling;
			n->sibling = p->sibling;
		}
	}
	
	free(p);
}









