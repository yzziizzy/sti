#ifndef __sti__b64_h__
#define __sti__b64_h__

#include <stdint.h>



void base64_decode(char* in, uint64_t inLen, uint8_t* out, uint64_t* outLen);
void base64_encode(unsigned char* in, uint64_t inLen, char* out, uint64_t* outLen);



#endif // __sti__b64_h__
