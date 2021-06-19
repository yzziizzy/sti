// Public Domain

#ifndef __sti_talloc_h__included__
#define __sti_talloc_h__included__

#include <stddef.h>

void* talloc(void* parent, size_t size);
void* trealloc(void* old, size_t size);
void  tfree(void* m);


#endif // __sti_talloc_h__included__
