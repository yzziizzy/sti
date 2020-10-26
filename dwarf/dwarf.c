#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include "dwarf.h"
#include "../sti.h"

// LEB128: "Little-Endian Base 128"


int uleb128_encode(uint64_t n, uint8_t* out) {
	uint8_t chunks[9];
	
	uint8_t a;
	int i = 0;
	do {
		a = n & 0x7f;
		n >>= 7;
		if(n != 0) a |= 0x80;
		out[i++] = a;
	} while(a != 0);
	
	return i;
}


uint64_t uleb128_decode(uint8_t* in, uint8_t** end) {
	uint64_t out = 0;
	int i;
	
	for(i = 0; 1; i++) {
		uint8_t a = in[i] & 0x7f;
		out |= a << (i * 7);
		
		if((in[i] & 0x80) == 0) break; 
	}
	
	if(end) *end = in + i + 1;
	
	return out;
}

int leb128_encode(int64_t n, uint8_t* out) {
	int i = 0;
	int neg = n < 0;
	
	// C does not specify if signed right shifts are arithmetic or logical
	uint64_t u = *((uint64_t*)&n);
	
	while(1) {
		uint8_t a = u & 0x7f;
		u >>= 7;
		
		if(neg) u |= -(1l<< 57l);
		
		out[i++] = a | 0x80;
		
		if(u == 0 && ((a & 0x40) == 0)) break;
		if(u == -1 && ((a & 0x40) == 0x40)) break;
	}
	
	// last byte has high bit cleared
	out[i - 1] &= 0x7f;
	
	return i;
}


int64_t leb128_decode(uint8_t* in, uint8_t** end) {
	int64_t sout;
	uint64_t out = 0;
	uint8_t a;
	int i;
	int sh = 0;
	
	for(i = 0; 1; i++) {
		a = in[i] & 0x7f;
		out |= a << sh;
		sh += 7;
		
		if((in[i] & 0x80) == 0) break; 
	}
	
	// sign extend an originally-negative number
	if((i < 8) && (a & 0x40) == 0x40) {
		out |= -(1l << sh); 
	}
	
	sout = *((int64_t*)&out);
	if(end) *end = in + i + 1;
	
	return sout;
}

#define PEEK_u8(p) (*((uint8_t*)(p)))
#define PEEK_u16(p) (*((uint16_t*)(p)))
#define PEEK_u32(p) (*((uint32_t*)(p)))
#define PEEK_u64(p) (*((uint64_t*)(p)))
#define PEEK_s8(p) (*((int8_t*)(p)))
#define PEEK_s16(p) (*((int16_t*)(p)))
#define PEEK_s32(p) (*((int32_t*)(p)))
#define PEEK_s64(p) (*((int64_t*)(p)))

struct fileinfo {
	char* name;
	uint64_t dir_index;
	uint64_t mtime;
	uint64_t size;
};

struct liheader {
	uint8_t* raw;
	size_t* raw_len;
	
	uint8_t header_bits; // 32 or 64
	uint8_t* header_part2; // len of this first part with version info
	
	uint64_t initial_length;
	uint16_t version;
	uint64_t header_length;
	
	/* ---- header_part2 ------- */
	uint8_t address_size;
	uint8_t segment_selector_size;
	uint8_t min_instr_length;
	uint8_t max_ops_per_instr;
	uint8_t default_is_stmt;
	int8_t line_base; // min. added to line for special opcode
	uint8_t line_range; // max. plus above - 1
	uint8_t opcode_base; // number assigned to first special opcode
	uint8_t* std_op_lengths; /*[opcode_base-1]*/
// 	uint8_t dir_entry_format_cnt;
// 	uint64_t*  dir_entry_format; /*[dir_entry_format_cnt]*/
// 	uint64_t dir_count;
// 	/*dir_entry_format*/ char* directories; /*[dir_count];*/
// 	uint64_t file_name_entry_fmt_cnt;
// 	uint64_t* file_name_entry_fmt; /*[file_name_entry_fmt_cnt];*/
// 	uint64_t file_names_cnt;
	/*file_name_entry_fmt*/ char* file_names; /*[file_names_cnt];*/
	
	
	VEC(char*) dir_names;
	VEC(struct fileinfo) files;
	
} ;





void* line_num_machine_v2(struct liheader* h) {
	uint8_t* raw = h->raw;
	uint8_t* r = h->header_part2;
	size_t len = h->raw_len;
	
	
	h->header_length = PEEK_u32(r);
	r += 4;
	h->min_instr_length = PEEK_u8(r);
	r++;
	h->default_is_stmt = PEEK_u8(r);
	r++;
	h->line_base = PEEK_s8(r);
	r++;
	h->line_range = PEEK_u8(r);
	r++;
	h->opcode_base = PEEK_u8(r);
	r++;
	
	printf("section len %d\n", len);
	printf("header_length %d\n", h->header_length);
	printf("min_instr_length %d\n", h->min_instr_length);
	printf("default_is_stmt %d\n", h->default_is_stmt);
	printf("line_base %d\n", h->line_base);
	printf("line_range %d\n", h->line_range);
	printf("opcode_base %d\n", h->opcode_base);
	
	h->std_op_lengths = malloc(sizeof(*h->std_op_lengths) * h->opcode_base);
	for(int i = 0; i < h->opcode_base-1; i++) {
		h->std_op_lengths[i] = PEEK_u8(r);
		r++;
		printf("oplen %d: %ld\n", i, h->std_op_lengths[i]);
	}
	printf("foo\n");
	// include_directories
	// null-terminated string list, followed by a null
	while(*r) {
		char* f;
		f = strdup(r);
		r += strlen(f) + 1;
		
		printf("dir: %s\n", f);
		VEC_PUSH(&h->dir_names, f);
	}
	r++;
// 	r = memmem(r, h->initial_length, "\0", 2) + 2;
	
	// file_names, 1-based
	// null string, uleb=dir_index, uleb=mtime, uleb=flen
	// final null byte
	// dir-index is 1-based; 0 = cur dir
	while(*r) {
		struct fileinfo f;
		f.name = strdup(r);
		r += strlen(f.name) + 1;
		printf("r %p\n", r);
		f.dir_index = uleb128_decode(r, &r);
		printf("r %p\n", r);
		f.mtime = uleb128_decode(r, &r);
		printf("r %p\n", r);
		f.size = uleb128_decode(r, &r);
		printf("r %p\n", r);
		
		printf("file: %s\n", f.name);
		VEC_PUSH(&h->files, f);
	}
	r++;
	
	// program start
	printf("prog start\n");
	
#define RESET_REGS() \
	address = 0; \
	op_index = 0; \
	column = 0; \
	line = 1; \
	file = 1; \
	is_stmt = h->default_is_stmt; \
	basic_block = 0; \
	end_sequence = 0; \
	prologue_end = 0; \
	epilogue_begin = 0; \
	isa = 0; \
	discriminator = 0;
	
	size_t address = 0;
	size_t op_index = 0;
	size_t column = 0;
	size_t line = 1;
	size_t file = 1;
	int is_stmt = h->default_is_stmt; // need to look in the header
	int basic_block = 0;
	int end_sequence = 0;
	int prologue_end = 0;
	int epilogue_begin = 0;
	size_t isa = 0;
	size_t discriminator = 0;

	size_t out_i = 0;
#define APPEND_CURRENT() printf("  row %ld addr:%lx, file:%ld, line:%ld, col:%ld\n", out_i++, address, file, line, column);
	
	uint8_t* start_prog = h->header_part2 + h->header_length + 4;
	uint8_t* prog = start_prog;
	int f = 0;
	
	while(prog <= raw + len) {
		uint8_t op = *prog;
		prog++;
		
		printf("\nloop:%d  op=%d  prog=%ld \n", f++, op, prog - start_prog);
		
		if(op >= h->opcode_base) { 
			printf("  special op\n");
			// special op decoding:
			
			uint8_t adj_op = op - h->opcode_base;
			
			uint64_t op_adv = adj_op / h->line_range;
			
			address += h->min_instr_length * op_adv;
// 			op_index = (op_index + op_adv) % h->max_ops_per_instr;
			line += h->line_base + (adj_op % h->line_range);
			
			APPEND_CURRENT()
			
			basic_block = 0;
			
			continue;
		}
		
		if(op == 0) {
			
			
			// extended opcode
			uint64_t l = uleb128_decode(prog, &prog);
			
			uint8_t* orig_prog = prog;
			op = *prog;
			prog++;
			printf("extended op: %d %ld\n", op, l);
			
			
			// extended opcodes
			switch(op) {
				case DW_LNE_end_sequence:
					end_sequence = 1;
					
					APPEND_CURRENT();
					
					RESET_REGS();
					
					return NULL;
					break;
					
				case DW_LNE_set_address:
					// 1 machine address (64bit) > address
					address = *((uint64_t*)prog);
					prog += 8;
					op_index = 0;
					
					break;
					
				case DW_LNE_set_discriminator:
					// 1 uleb > discriminator 
					discriminator = uleb128_decode(prog, &prog);
					break;
					
				
				case DW_LNE_define_file:
					printf("define file ext. opcode\n");
					printf("  '%s' \n", prog);
					// null string w/ file name
					// uleb dir index
					// uleb mtime
					// uleb fsize
					exit(1);
					break;
				
			}
			
			prog = orig_prog + l;
			continue;
		}
		
		switch(op) {
			case DW_LNS_copy:
				// no operands
				
				APPEND_CURRENT();
				
				discriminator = 0;
				basic_block = 0;
				prologue_end = 0;
				epilogue_begin = 0;
				break;
				
			case DW_LNS_advance_pc: {
				//  1 uleb operand
				uint64_t op_adv = uleb128_decode(prog, &prog);
				
				address += h->min_instr_length * op_adv;
				
				break;
			}
			case DW_LNS_advance_line:
				//  1 leb operand
				line += leb128_decode(prog, &prog);
				break;
				
			case DW_LNS_set_file:
				//  1 uleb operand > file
				file = uleb128_decode(prog, &prog);
				break;
				
			case DW_LNS_set_column:
				//  1 uleb operand > column
				column = uleb128_decode(prog, &prog);
				break;
				
			case DW_LNS_negate_stmt:
				is_stmt = !is_stmt;
				break;
				
			case DW_LNS_set_basic_block:
				basic_block = 1;
				break;
				
			case DW_LNS_const_add_pc: {
				// adv. address and op_index per sp.op. #255
				int adv = 255 - h->opcode_base;
				
				address += h->min_instr_length * adv;
// 				op_index = (op_index + adv) % h->max_ops_per_instr;
				break;
			}
			case DW_LNS_fixed_advance_pc:
				// 1 uhalf op (unencoded) += address
				address += *((uint16_t*)prog);
				prog += 2;
// 				op_index = 0;
				break;
				
			case DW_LNS_set_prologue_end:
				prologue_end = 1;
				break;
				
			case DW_LNS_set_epilogue_begin:
				epilogue_begin = 1;
				break;
				
			case DW_LNS_set_isa:
				// 1 uleb > isa
				isa = uleb128_decode(prog, &prog);
				break;
				
			
			
				
			default:
				// read oplen, then skip forward
				printf("unknown op: %d\n", op);
				/*
				for(int i = 0; i < oplens[op]; i++) {
					uleb128_decode(prog, &prog);
				}
				break;
				*/
		}
		
	
	}
}




void* line_num_machine_v4(struct liheader* h) {
	
	uint8_t* raw = h->raw;
	size_t len = h->raw_len;
	
	
	/*
	if(h.header_bits == 32) {
		h.header_length = PEEK_u32(r);
		r += 4;
	}
	else {
		h.header_length = PEEK_u64(r);
		r += 8;
	}*/
// 	printf("initial length: %d\n", h->initial_length);
// 	printf("header length: %d\n", h->header_length);
// 	printf("opcode_base: %d\n", h->opcode_base);
	
	// translate the standard opcode operand counts to a sane number format (because this is /totally/ worth saving 27 bytes in a /debug build/ executable 
	int64_t* oplens = malloc(sizeof(oplens) * h->opcode_base);
	uint8_t* hp = raw + sizeof(*h);
	for(int i = 0; i < h->opcode_base; i++) {
		oplens[i] = leb128_decode(hp, &hp);
	}
	
	int dir_entry_format_cnt = *hp;
	hp++;
	for(int i = 0; i < dir_entry_format_cnt; i++) {
		uleb128_decode(hp, &hp); // content type code
		uleb128_decode(hp, &hp); // form code
	}
	
	uint64_t dir_cnt = uleb128_decode(hp, &hp); // these 2 bytes saved allowed the executable to fit on my 6TB disk.
// 	for(int i = 0; i < dir_cnt; i++) { // TODO: complicated variable format
// 		uleb128_decode(hp, &hp); // content type code
// 		uleb128_decode(hp, &hp); // form code
// 	}
	
	//.. can't do any more until the complicated format is done
	
#define RESET_REGS() \
	address = 0; \
	op_index = 0; \
	column = 0; \
	line = 1; \
	file = 1; \
	is_stmt = h->default_is_stmt; \
	basic_block = 0; \
	end_sequence = 0; \
	prologue_end = 0; \
	epilogue_begin = 0; \
	isa = 0; \
	discriminator = 0;
	
	size_t address = 0;
	size_t op_index = 0;
	size_t column = 0;
	size_t line = 1;
	size_t file = 1;
	int is_stmt = h->default_is_stmt; // need to look in the header
	int basic_block = 0;
	int end_sequence = 0;
	int prologue_end = 0;
	int epilogue_begin = 0;
	size_t isa = 0;
	size_t discriminator = 0;

	size_t out_i = 0;
#define APPEND_CURRENT() printf("  row %ld addr:%lx, file:%ld, line:%ld, col:%ld\n", out_i++, address, file, line, column);
	
	
	uint8_t* start_prog = &h->header_length + h->header_length;
	uint8_t* prog = &h->header_length + h->header_length;
	int f = 0;
	
	while(prog <= raw + len) {
		uint8_t op = *prog;
		prog++;
		
		printf("\nloop:%d  op=%d  prog=%ld \n", f++, op, prog-start_prog);
		
		if(op >= h->opcode_base) { 
			printf("  special op\n");
			// special op decoding:
			
			uint8_t adj_op = op - h->opcode_base;
			
			uint64_t op_adv = adj_op / h->line_range;
			
			address += h->min_instr_length + ((op_index + op_adv) / h->max_ops_per_instr);
			op_index = (op_index + op_adv) % h->max_ops_per_instr;
			line += h->line_base + (adj_op % h->line_range);
			
			APPEND_CURRENT()
			
			basic_block = 0;
			prologue_end = 0;
			epilogue_begin = 0;
			discriminator = 0;
			
			continue;
		}
		
		if(op == 0) {
			// extended opcode
			uint64_t l = uleb128_decode(prog, &prog);
			uint8_t orig_prog = prog;
			op = *prog;
			
			// extended opcodes
			switch(op) {
				case DW_LNE_end_sequence:
					end_sequence = 1;
					
					APPEND_CURRENT();
					
					RESET_REGS();
					
					return NULL;
					break;
					
				case DW_LNE_set_address:
					// 1 machine address (64bit) > address
					address = *((uint64_t*)prog);
					prog += 8;
					op_index = 0;
					
					break;
					
				case DW_LNE_set_discriminator:
					// 1 uleb > discriminator 
					discriminator = uleb128_decode(prog, &prog);
					break;
					
				/*
				case DW_LNS_define_file:
					// deprecated as of DWARF v5
					printf("DW_LNS_define_file is deprecated.\n");
					break;
				*/
			}
			
			prog = orig_prog + l;
			continue;
		}
		
		switch(op) {
			case DW_LNS_copy:
				// no operands
				
				APPEND_CURRENT();
				
				discriminator = 0;
				basic_block = 0;
				prologue_end = 0;
				epilogue_begin = 0;
				break;
				
			case DW_LNS_advance_pc: {
				//  1 uleb operand
				uint64_t adv = uleb128_decode(prog, &prog);
				// mod address and op_index per rules on p160
				
				address += h->min_instr_length + ((op_index + adv) / h->max_ops_per_instr);
				op_index = (op_index + adv) % h->max_ops_per_instr;
				
				break;
			}
			case DW_LNS_advance_line:
				//  1 leb operand
				line += leb128_decode(prog, &prog);
				break;
				
			case DW_LNS_set_file:
				//  1 uleb operand > file
				file = uleb128_decode(prog, &prog);
				break;
				
			case DW_LNS_set_column:
				//  1 uleb operand > column
				column = uleb128_decode(prog, &prog);
				break;
				
			case DW_LNS_negate_stmt:
				is_stmt = !is_stmt;
				break;
				
			case DW_LNS_set_basic_block:
				basic_block = 1;
				break;
				
			case DW_LNS_const_add_pc: {
				// adv. address and op_index per sp.op. #255
				int adv = 255 - h->opcode_base;
				
				address += h->min_instr_length + ((op_index + adv) / h->max_ops_per_instr);
				op_index = (op_index + adv) % h->max_ops_per_instr;
				break;
			}
			case DW_LNS_fixed_advance_pc:
				// 1 uhalf op (unencoded) += address
				address += *((uint16_t*)prog);
				prog += 2;
				op_index = 0;
				break;
				
			case DW_LNS_set_prologue_end:
				prologue_end = 1;
				break;
				
			case DW_LNS_set_epilogue_begin:
				epilogue_begin = 1;
				break;
				
			case DW_LNS_set_isa:
				// 1 uleb > isa
				isa = uleb128_decode(prog, &prog);
				break;
				
			
			
				
			default:
				// read oplen, then skip forward
				printf("unknown op: %d\n", op);
				
				for(int i = 0; i < oplens[op]; i++) {
					uleb128_decode(prog, &prog);
				}
				break;
		}
		
	
	}
}







void* line_num_machine(uint8_t* raw, size_t len) {
	struct liheader h = {0};
	uint8_t* r = raw;
	
	h.raw = raw;
	h.raw_len = len;
	h.initial_length = PEEK_u32(r);
	r += 4;
	
	if(h.initial_length >= 0xfffffff0) {
		// 64 bit header
		h.header_bits = 64;
		h.initial_length = PEEK_u64(r);
		r += 8;
	}
	else {
		// 32 bit header
		h.header_bits = 32;
	}
	
	h.version = PEEK_u16(r);
	r += 2;
	
	
	
	h.header_part2 = r; // versioned functions pick up here
	
// 	printf("line number version: %d\n", h.version);
// 	
	switch(h.version) {
		case 2: return line_num_machine_v2(&h);
// 		case 4: return line_num_machine_v4(&h);
		default:
			printf("Unsupported DWARF Line Info version: %d\n", h.version);
			return NULL;
	}
	
}



void* debug_info_parse(uint8_t* raw, size_t raw_len) {
	
	int filebits;
	uint8_t* r = raw;
	uint64_t debug_abbrev_offset;
	uint8_t address_bytes;
	
// 	h.raw = raw;
// 	h.raw_len = len;
	uint64_t initial_length = PEEK_u32(r);
	r += 4;
	
	if(initial_length >= 0xfffffff0) {
		// 64 bit header
		h.filebits = 64;
// 		h.initial_length = PEEK_u64(r);
		r += 8;
	}
	else {
		// 32 bit header
		filebits = 32;
	}
	
	uint16_t version = PEEK_u16(r);
	r += 2;
	printf("debug info version: %d \n", version);
	
	if(version != 4) {
		printf("unsupported debug info version: %d\n", version);
		return NULL;
	}
	
	if(filebits == 64) {
		debug_abbrev_offset = PEEK_u64(r);
		r += 8;
	}
	else {
		debug_abbrev_offset = PEEK_u32(r);
		r += 4;
	}
	
	address_bytes = PEEK_u8(r);
	r++;
	
	
	
}


