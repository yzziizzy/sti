#ifndef __sti__b64_h__
#define __sti__b64_h__

#include <stdint.h>



void base64_decode(char* in, uint64_t inLen, uint8_t* out, uint64_t* outLen);
void base64_encode(unsigned char* in, uint64_t inLen, char* out, uint64_t* outLen);

// GIGO; only feed in valid strings with no filler chars
// the 65th character is what '=' normally is
void base64_decode_custom(const char alpha[65], char* in, uint64_t inLen, uint8_t* out, uint64_t* outLen);
void base64_encode_custom(const char alpha[65], unsigned char* in, uint64_t inLen, char* out, uint64_t* outLen);



#endif // __sti__b64_h__
