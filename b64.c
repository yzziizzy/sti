


#include "b64.h"



static inline uint8_t b64char(uint8_t c) {
	if(c >= 'a') return c - 'a' + 26;
	if(c >= 'A') return c - 'A';
	if(c >= '0') return c - '0' + 52;
	if(c == '/') return 63;
	if(c == '+') return 62;
	return 0;
}


void base64_decode(char* in, uint64_t inLen, uint8_t* out, uint64_t* outLen) {
	
	uint64_t quads = inLen / 4;
	uint32_t rem = inLen % 4;
	
	uint64_t o = 0;
	
	uint64_t i = 0;
	for(uint64_t j = 0; j < quads; j++, i += 4) {
		union {
			uint64_t q;
			uint8_t b[4];
		} x;
			
		uint8_t c0 = b64char(in[i + 0]);
		uint8_t c1 = b64char(in[i + 1]);
		uint8_t c2 = b64char(in[i + 2]);
		uint8_t c3 = b64char(in[i + 3]);
		
		x.q = (c0 << 18) | (c1 << 12) | (c2 << 6) | c3;
		
		out[o + 0] = x.b[2];
		out[o + 1] = x.b[1];
		out[o + 2] = x.b[0];
		
		o += 3;
	}
	
	*outLen = quads * 3;
	
	// adjust length for padding
	if(in[i - 2] == '=') (*outLen) -= 2;
	else if(in[i - 1] == '=') (*outLen)--;
	
	if(!rem) return;
	
	uint8_t c0 = 0, c1 = 0, c2 = 0;
	union {
		uint64_t q;
		uint8_t b[4];
	} x;
	
	x.q = 0;
	
	switch(rem) {
		case 3:
			c2 = b64char(in[i + 2]);
			/* fallthrough */
		case 2:
			c1 = b64char(in[i + 1]);
			/* fallthrough */
		case 1:
			c0 = b64char(in[i + 0]);
		default:
	}
	
	x.q = (c0 << 18) | (c1 << 12) | (c2 << 6);
	
	
	if(x.b[0]) {
		out[o + 2] = x.b[0];
		(*outLen)++;
	}
	if(x.b[1]) {
		out[o + 1] = x.b[1];
		(*outLen)++;
	}
	if(x.b[2]) {
		out[o + 0] = x.b[2];
		(*outLen)++;
	}
}


static inline char charfromhextet(uint8_t h) {
	if(h < 26) return h + 'A';
	if(h < 52) return h - 26 + 'a';
	if(h < 62) return h - 52 + '0';
	if(h == 62) return '+';
	if(h == 63) return '/';
	return 0;
}


void base64_encode(unsigned char* in, uint64_t inLen, char* out, uint64_t* outLen) {
	
	uint64_t trios = inLen / 3;
	uint64_t rem = inLen % 3;
	
	uint64_t o = 0;
	
	uint64_t i = 0;
	for(uint64_t j = 0; j < trios; j++, i += 3) {
		uint32_t x = 0;
		
		u32 i0 = in[i + 0];
		u32 i1 = in[i + 1];
		u32 i2 = in[i + 2];
		
		i0 <<= 16;
		i1 <<= 8;
		
		i0 &= 0x00ff0000;
		i1 &= 0x0000ff00;
		
		x = i0 | i1 | i2;
		
		out[o + 0] = charfromhextet((x >> 18) & 0b00111111);
		out[o + 1] = charfromhextet((x >> 12) & 0b00111111);
		out[o + 2] = charfromhextet((x >> 6) & 0b00111111);
		out[o + 3] = charfromhextet((x >> 0) & 0b00111111);
		
		o += 4;
	}
	
	*outLen = trios * 4;
	
	if(!rem) return;
	(*outLen) += 4;
	
	out[o + 3] = '=';
	
	uint32_t x = 0;
	x = (in[i + 0] << 16);
	
	if(rem == 2) {
		x |= (in[i + 1] << 8);
		out[o + 2] = charfromhextet((x >> 6) & 0b00111111);
	}
	else {
		out[o + 2] = '=';
	}
	
	out[o + 0] = charfromhextet((x >> 18) & 0b00111111);
	out[o + 1] = charfromhextet((x >> 12) & 0b00111111);
	
}






static inline uint8_t b64char_custom(char alpha[64], uint8_t c) {
	for(int i = 0; i < 64; i++) {
		if(alpha[i] == c) return i;
	};
	return 0;
}

// the 65th character is what '=' normally is
// GIGO; only feed in valid strings with no filler chars
void base64_decode_custom(const char alpha[65], char* in, uint64_t inLen, uint8_t* out, uint64_t* outLen) {
	
	uint64_t quads = inLen / 4;
	uint32_t rem = inLen % 4;
	
	uint64_t o = 0;
	
	uint64_t i = 0;
	for(uint64_t j = 0; j < quads; j++, i += 4) {
		union {
			uint64_t q;
			uint8_t b[4];
		} x;
			
		uint8_t c0 = b64char(in[i + 0]);
		uint8_t c1 = b64char(in[i + 1]);
		uint8_t c2 = b64char(in[i + 2]);
		uint8_t c3 = b64char(in[i + 3]);
		
		x.q = (c0 << 18) | (c1 << 12) | (c2 << 6) | c3;
		
		out[o + 0] = x.b[2];
		out[o + 1] = x.b[1];
		out[o + 2] = x.b[0];
		
		o += 3;
	}
	
	*outLen = quads * 3;
	
	// adjust length for padding
	if(in[i - 2] == alpha[64]) (*outLen) -= 2;
	else if(in[i - 1] == alpha[64]) (*outLen)--;
	
	if(!rem) return;
	
	uint8_t c0 = 0, c1 = 0, c2 = 0;
	union {
		uint64_t q;
		uint8_t b[4];
	} x;
	
	x.q = 0;
	
	switch(rem) {
		case 3:
			c2 = b64char(in[i + 2]);
			/* fallthrough */
		case 2:
			c1 = b64char(in[i + 1]);
			/* fallthrough */
		case 1:
			c0 = b64char(in[i + 0]);
		default:
	}
	
	x.q = (c0 << 18) | (c1 << 12) | (c2 << 6);
	
	
	if(x.b[0]) {
		out[o + 2] = x.b[0];
		(*outLen)++;
	}
	if(x.b[1]) {
		out[o + 1] = x.b[1];
		(*outLen)++;
	}
	if(x.b[2]) {
		out[o + 0] = x.b[2];
		(*outLen)++;
	}
}



// GIGO
void base64_encode_custom(const char alpha[65], unsigned char* in, uint64_t inLen, char* out, uint64_t* outLen) {
	
	uint64_t trios = inLen / 3;
	uint64_t rem = inLen % 3;
	
	uint64_t o = 0;
	
	uint64_t i = 0;
	for(uint64_t j = 0; j < trios; j++, i += 3) {
		uint32_t x = 0;
		
		u32 i0 = in[i + 0];
		u32 i1 = in[i + 1];
		u32 i2 = in[i + 2];
		
		i0 <<= 16;
		i1 <<= 8;
		
		i0 &= 0x00ff0000;
		i1 &= 0x0000ff00;
		
		x = i0 | i1 | i2;
		
		out[o + 0] = alpha[(x >> 18) & 0b00111111];
		out[o + 1] = alpha[(x >> 12) & 0b00111111];
		out[o + 2] = alpha[(x >> 6) & 0b00111111];
		out[o + 3] = alpha[(x >> 0) & 0b00111111];
		
		o += 4;
	}
	
	*outLen = trios * 4;
	
	if(!rem) return;
	(*outLen) += 4;
	
	out[o + 3] = alpha[64];
	
	uint32_t x = 0;
	x = (in[i + 0] << 16);
	
	if(rem == 2) {
		x |= (in[i + 1] << 8);
		out[o + 2] = alpha[(x >> 6) & 0b00111111];
	}
	else {
		out[o + 2] = alpha[64];
	}
	
	out[o + 0] = alpha[(x >> 18) & 0b00111111];
	out[o + 1] = alpha[(x >> 12) & 0b00111111];
	
}





