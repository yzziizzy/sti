#ifndef __sti__ini_h__
#define __sti__ini_h__

// Public Domain.



typedef int (*ini_read_callback_fn)(char*, char*, char*, void*);



void ini_read(char* path, ini_read_callback_fn fn, void* user_data);



#endif // __sti__ini_h__
