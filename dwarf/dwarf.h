#ifndef __sti_dwarf_dwarf_h__
#define __sti_dwarf_dwarf_h__


#include <stdint.h>



#define DWARF_ENUM_PREFIXES \
	X(ACCESS) \
	X(AT) \
	X(ATE) \
	X(CC) \
	X(CFA) \
	X(CHILDREN) \
	X(DEFAULTED) \
	X(DS) \
	X(DSC) \
	X(END) \
	X(FORM) \
	X(ID) \
	X(IDX) \
	X(INL) \
	X(LANG) \
	X(LLE) \
	X(LNCT) \
	X(LNS) \
	X(MACRO) \
	X(OP) \
	X(ORD) \
	X(RLE) \
	X(SECT) \
	X(TAG) \
	X(UT) \
	X(VIRTUALITY) \
	X(VIS)

#include "dwarf_enums.h"

enum {
	#define X(prefix, name, val) DW_ ## prefix ## _ ## name = val,
	DWARF_ENUM_PAIRS
	#undef X
};






// LEB128: "Little-Endian Base 128"

uint64_t uleb128_decode(uint8_t* in , uint8_t** end);
int64_t leb128_decode(uint8_t* in , uint8_t** end);

// returns the number of btyes written
int uleb128_encode(uint64_t n, uint8_t* out);
int leb128_encode(int64_t n, uint8_t* out);

void line_num_machine(uint8_t* raw, size_t len);


#endif // __sti_dwarf_dwarf_h__
