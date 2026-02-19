#ifndef __sti__debug_h__
#define __sti__debug_h__

#include <stdarg.h>
#include <stdint.h>

//#define STI_DEBUG_PATH_PREFIX "src/" or whatever your prefix is


typedef int8_t*  int8p_t;
typedef int16_t* int16p_t;
typedef int32_t* int32p_t;
typedef int64_t* int64p_t;
typedef uint8_t*  uint8p_t;
typedef uint16_t* uint16p_t;
typedef uint32_t* uint32p_t;
typedef uint64_t* uint64p_t;
typedef float* floatp_t;
typedef double* doublep_t;
typedef char* charp_t;
typedef char** charpp_t;



#if !defined(C3DLAS_DEBUG_TYPE_LIST)
	#if defined(HAS_C3DLAS)
		
		typedef Vector2* Vector2p; 
		typedef Vector3* Vector3p; 
		typedef Vector4* Vector4p; 
		typedef Vector2i* Vector2ip; 
		typedef Vector3i* Vector3ip; 
		typedef Vector4i* Vector4ip; 
		typedef Vector2l* Vector2lp; 
		typedef Vector3l* Vector3lp; 
		typedef Vector4l* Vector4lp; 
		typedef Vector2d* Vector2dp; 
		typedef Vector3d* Vector3dp; 
		typedef Vector4d* Vector4dp; 
		typedef Matrix3* Matrix3p; 
		typedef Matrix4* Matrix4p; 
		
		
		#define C3DLAS_DEBUG_TYPE_LIST(X) \
			X(Vector2i,    1, 0, 1, 0, 4, 2, 0) \
			X(Vector3i,    1, 0, 1, 0, 4, 3, 0) \
			X(Vector4i,    1, 0, 1, 0, 4, 4, 0) \
			X(Vector2l,    1, 0, 1, 0, 8, 2, 0) \
			X(Vector3l,    1, 0, 1, 0, 8, 3, 0) \
			X(Vector4l,    1, 0, 1, 0, 8, 4, 0) \
			X(Vector2,     1, 1, 0, 0, 4, 2, 0) \
			X(Vector3,     1, 1, 0, 0, 4, 3, 0) \
			X(Vector4,     1, 1, 0, 0, 4, 4, 0) \
			X(Vector2d,    1, 1, 0, 0, 8, 2, 0) \
			X(Vector3d,    1, 1, 0, 0, 8, 3, 0) \
			X(Vector4d,    1, 1, 0, 0, 8, 4, 0) \
			X(Matrix4,     1, 1, 0, 0, 4,16, 0) \
			X(Matrix3,     1, 1, 0, 0, 4, 9, 0) \
			X(Vector2ip,   1, 0, 1, 0, 4, 2, 1) \
			X(Vector3ip,   1, 0, 1, 0, 4, 3, 1) \
			X(Vector4ip,   1, 0, 1, 0, 4, 4, 1) \
			X(Vector2lp,   1, 0, 1, 0, 8, 2, 1) \
			X(Vector3lp,   1, 0, 1, 0, 8, 3, 1) \
			X(Vector4lp,   1, 0, 1, 0, 8, 4, 1) \
			X(Vector2p,    1, 1, 0, 0, 4, 2, 1) \
			X(Vector3p,    1, 1, 0, 0, 4, 3, 1) \
			X(Vector4p,    1, 1, 0, 0, 4, 4, 1) \
			X(Vector2dp,   1, 1, 0, 0, 8, 2, 1) \
			X(Vector3dp,   1, 1, 0, 0, 8, 3, 1) \
			X(Vector4dp,   1, 1, 0, 0, 8, 4, 1) \
			X(Matrix4p,    1, 1, 0, 0, 4,16, 1) \
			X(Matrix3p,    1, 1, 0, 0, 4, 9, 1)
			
	#else 
		#define C3DLAS_DEBUG_TYPE_LIST(X)
	#endif
#endif



//   type        float  str   vlen
//           signed  int   size  ptrlvl
#define STI_DEBUG_TYPE_LIST(X) \
	X(int8_t,        1, 0, 1, 0, 1, 1, 0) \
	X(int16_t,       1, 0, 1, 0, 2, 1, 0) \
	X(int32_t,       1, 0, 1, 0, 4, 1, 0) \
	X(int64_t,       1, 0, 1, 0, 8, 1, 0) \
	X(uint8_t,       0, 0, 1, 0, 1, 1, 0) \
	X(uint16_t,      0, 0, 1, 0, 2, 1, 0) \
	X(uint32_t,      0, 0, 1, 0, 4, 1, 0) \
	X(uint64_t,      0, 0, 1, 0, 8, 1, 0) \
	X(float_t,       1, 1, 0, 0, 4, 1, 0) \
	X(double_t,      1, 1, 0, 0, 8, 1, 0) \
	X(charp_t,       0, 0, 0, 1, 8, 1, 0) \
	X(int8p_t,       1, 0, 1, 0, 1, 1, 1) \
	X(int16p_t,      1, 0, 1, 0, 2, 1, 1) \
	X(int32p_t,      1, 0, 1, 0, 4, 1, 1) \
	X(int64p_t,      1, 0, 1, 0, 8, 1, 1) \
	X(uint8p_t,      0, 0, 1, 0, 1, 1, 1) \
	X(uint16p_t,     0, 0, 1, 0, 2, 1, 1) \
	X(uint32p_t,     0, 0, 1, 0, 4, 1, 1) \
	X(uint64p_t,     0, 0, 1, 0, 8, 1, 1) \
	X(floatp_t,      1, 1, 0, 0, 4, 1, 1) \
	X(doublep_t,     1, 1, 0, 0, 8, 1, 1) \
	X(charpp_t,      0, 0, 0, 1, 8, 1, 1) \
	C3DLAS_DEBUG_TYPE_LIST(X)






#define X(a, ...) union STI_DBG_TYPE_##a {};
	STI_DEBUG_TYPE_LIST(X)
#undef X

enum {
	STI_DBG_TYPE_NONE = 0,
#define X(a, ...) STI_DBG_TYPE_##a,
	STI_DEBUG_TYPE_LIST(X)
#undef X
	STI_DBG_TYPE_MAX_VALUE,
};




typedef struct sti_DebugTypeInfo {
	char* name;
	char is_signed;
	char is_float;
	char is_int;
	char is_string;
	char size;
	char vec_len;
	char ind_level;
} sti_DebugTypeInfo;

extern sti_DebugTypeInfo sti_debug_type_info[STI_DBG_TYPE_MAX_VALUE];

struct sti__useless_type {};

#define STI_WATCH_TYPE(a, ...) a: STI_DBG_TYPE_##a,
#define STI_WATCH_TYPE_TO_ENUM(a) _Generic(a, STI_DEBUG_TYPE_LIST(STI_WATCH_TYPE) struct sti__useless_type: 0)


#ifdef STI_DEBUG_PATH_PREFIX
	#define dbg(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", strstr(__FILE__, STI_DEBUG_PATH_PREFIX) + strlen(STI_DEBUG_PATH_PREFIX), __LINE__ __VA_OPT__(,) __VA_ARGS__);
#else
	#define dbg(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__);
#endif

#define dbgv(a) sti_debug_print_var(__FILE__, __LINE__, __func__, #a, (void*)&a, STI_WATCH_TYPE_TO_ENUM(a));
void sti_debug_print_var(char* filename, long linenum, const char* funcname, char* name, void* var, int type);

//#define debug_break *((char*)0) = "debug break";




#endif // __sti__debug_h__
