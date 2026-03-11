#ifndef __sti__insanity_h__
#define __sti__insanity_h__

/*

This file is full of somewhat sketchy macros. Use if you want, ignore if you don't.

-- Not included in sti.h by default. You must include it manually. --


*/


// FOR(i, limit)
// FOR(i, step, limit)
#define FOR(v, ...) FOR__N(PP_NARG(__VA_ARGS__), v, __VA_ARGS__)
#define FOR__N(z, v, ...) CAT(FOR__, z)(v, __VA_ARGS__)
#define FOR__1(v, n) for(long v = 0; v < (n); v++)
#define FOR__2(v, s, n) for(long v = (s); v < (n); v++)


#define ciel(x) ceil(x)

#define clear(x) memset(&x, 0, sizeof(x))


#define I2D(p, sz)  ( (p).x + (p).y * (sz).x )
#define I3D(p, sz)  ( (p).x + (p).y * (sz).x + (p).z * (sz).x * (sz).y )


// the printf argument list of vectors
#define PRINTF_V2(q) (q).x, (q).y 
#define PRINTF_V3(q) (q).x, (q).y, (q).z 
#define PRINTF_V4(q) (q).x, (q).y, (q).z, (q).w








/*
Loop macro magic

https://www.chiark.greenend.org.uk/~sgtatham/mp/

HashTable obj;
HT_LOOP(&obj, key, char*, val) {
	printf("loop: %s, %s", key, val);
}

effective source:

	#define HT_LOOP(obj, keyname, valtype, valname)
	if(0)
		finished: ;
	else
		for(char* keyname;;) // internal declarations, multiple loops to avoid comma op funny business
		for(valtype valname;;)
		for(void* iter = NULL ;;)
			if(HT_next(obj, iter, &keyname, &valname))
				goto main_loop;
			else
				while(1)
					if(1) {
						// when the user uses break
						goto finished;
					}
					else
						while(1)
							if(!HT_next(obj, iter, &keyname, &valname)) {
								// normal termination
								goto finished;
							}
							else
								main_loop:
								//	{ user block; not in macro }
*/

// macro to make loop macros
// usage:
/*
#define JSON_ARR_LOOP_VAL_DECL(obj, valname)       json_value_t* valname
#define JSON_ARR_LOOP_ITER_DECL(obj, index)        json_link_t* link = (obj) ? (obj)->arr.head : 0
#define JSON_ARR_LOOP_ITER_INC(obj, index)         link = link->next
#define JSON_ARR_LOOP_DONE(obj, index)             link == NULL
#define JSON_ARR_LOOP_SET_VAL(obj, index, valname) valname = link->v
	
#define JSON_ARR_EACH(cat_j, i, val) \
	LOOP_HELPER_EACH(JSON_ARR_LOOP, cat_j, i, val)
*/



#define LOOP_HELPER__PASTE(a, b) CAT(a, b) 
#define LOOP_HELPER__ITER(key, val) LOOP_HELPER__PASTE(LOOP_HELPER_iter_ ## key ## __ ## val ## __, __LINE__)
#define LOOP_HELPER__FINISHED(key, val) LOOP_HELPER__PASTE(LOOP_HELPER_finished__ ## key ## __ ## val ## __, __LINE__)
#define LOOP_HELPER__MAINLOOP(key, val) LOOP_HELPER__PASTE(LOOP_HELPER_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define LOOP_HELPER_EACH(prefix, obj, index, valname) \
if(0) \
	LOOP_HELPER__FINISHED(index, valname): ; \
else \
	for(CAT(prefix, _VAL_DECL)(obj, valname);;) \
	for(CAT(prefix, _ITER_DECL)(obj, index);;) \
		if(!(CAT(prefix, _DONE)(obj, index)) && ((CAT(prefix, _SET_VAL)(obj, index, valname)), 1)) \
			goto LOOP_HELPER__MAINLOOP(index, valname); \
		else \
			while(1) \
				if(1) { \
					goto LOOP_HELPER__FINISHED(index, valname); \
				} \
				else \
					while(1) \
						if( (  ((CAT(prefix, _ITER_INC)(obj, index)), 0) ||  CAT(prefix, _DONE)(obj, index)  ) || ((CAT(prefix, _SET_VAL)(obj, index, valname)), 0)) { \
							goto LOOP_HELPER__FINISHED(index, valname); \
						} \
						else \
							LOOP_HELPER__MAINLOOP(index, valname) :
							
							//	{ user block; not in macro }


#define LOOP_HT_HELPER__PASTE(a, b) CAT(a, b) 
#define LOOP_HT_HELPER__ITER(key, val) LOOP_HT_HELPER__PASTE(LOOP_HT_HELPER_iter_ ## key ## __ ## val ## __, __LINE__)
#define LOOP_HT_HELPER__FINISHED(key, val) LOOP_HT_HELPER__PASTE(LOOP_HT_HELPER_finished__ ## key ## __ ## val ## __, __LINE__)
#define LOOP_HT_HELPER__MAINLOOP(key, val) LOOP_HT_HELPER__PASTE(LOOP_HT_HELPER_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define LOOP_HT_HELPER_EACH(prefix, obj, iter, keyname, valname) \
if(0) \
	LOOP_HT_HELPER__FINISHED(keyname, valname): ; \
else \
	for(CAT(prefix, _KEY_DECL)(obj, keyname);;) \
	for(CAT(prefix, _VAL_DECL)(obj, valname);;) \
	for(CAT(prefix, _ITER_DECL)(obj, iter);;) \
		if(CAT(prefix, _NEXT_CALL)(obj, iter, keyname, valname)) \
			goto LOOP_HT_HELPER__MAINLOOP(keyname, valname); \
		else \
			while(1) \
				if(1) { \
					goto LOOP_HT_HELPER__FINISHED(keyname, valname); \
				} \
				else \
					while(1) \
						if(!(CAT(prefix, _NEXT_CALL)(obj, iter, keyname, valname))) { \
							goto LOOP_HT_HELPER__FINISHED(keyname, valname); \
						} \
						else \
							LOOP_HT_HELPER__MAINLOOP(keyname, valname) :
							
							//	{ user block; not in macro }








#endif // __sti__insanity_h__
