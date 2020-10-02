#ifndef __sti_hash_fns_sha_h__
#define __sti_hash_fns_sha_h__

#include <stdint.h>

typedef struct sha256_state {
    uint64_t length;
    uint32_t state[8];
    uint32_t curlen;
    unsigned char buf[64];
} sha256_state;

typedef struct sha512_state {
    uint64_t length;
    uint64_t state[8];
    uint32_t curlen;
    unsigned char buf[128];
} sha512_state;


// sha224 is a truncated sha256
// sha384 is a truncated sha512


// out is 256 bits/32 bytes

// All in one go
void sha256_sum(const void* in, uint32_t inlen, uint8_t* out32);
void sha512_sum(const void* in, uint32_t inlen, uint8_t* out64);


// block by block processing
void sha256_init(sha256_state* md);
void sha256_process(sha256_state* md, const void* in, uint32_t inlen);
void sha256_done(sha256_state* md, void* out);

void sha512_init(sha512_state* md);
void sha512_process(sha512_state* md, const void* in, uint32_t inlen);
void sha512_done(sha512_state* md, void* out);




#endif // __sti_hash_fns_sha_h__
