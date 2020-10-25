#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include "dwarf.h"


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
	
	if(end) *end = in + i;
	
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
	if(end) *end = in + i;
	
	return sout;
}

void line_num_machine(uint8_t* raw, size_t len) {
	
	
	struct liheader {
		uint32_t initial_length;
// 		uint64_t initial_length;
		uint16_t version;
// 		uint8_t address_size;
// 		uint8_t segment_selector_size;
		uint32_t header_length;
		uint8_t min_instr_length;
		uint8_t max_ops_per_instr;
		uint8_t default_is_stmt;
		int8_t line_base; // min. added to line for special opcode
		uint8_t line_range; // max. plus above - 1
		uint8_t opcode_base; // number assigned to first special opcode
	//	uint8_t std_op_lengths[opcode_base];
	//	uint8_t dir_entry_format_cnt;
	//	uleb128[2]  dir_entry_format[dir_entry_format_cnt];
	//	uleb128 dir_count;
	//	dir_entry_format directories[dir_count];
	//	uint8_t file_name_entry_fmt_cnt;
	//	uleb128[2] file_name_entry_fmt[file_name_entry_fmt_cnt];
	//	uleb128 file_names_cnt;
	//	file_name_entry_fmt file_names[file_names_cnt];
		
	} __attribute__((packed));

	struct liheader* h = (struct liheader*)raw;
	
	printf("initial length: %d\n", h->initial_length);
	printf("header length: %d\n", h->header_length);
	
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
#define APPEND_CURRENT() printf("row %ld addr:%lx, file:%ld, line:%ld, col:%ld\n", out_i++, address, file, line, column);
	
	
	uint8_t* prog = &h->header_length + h->header_length;
	int f = 0;
	
	while(prog <= raw + len) {
		uint8_t op = *prog;
		prog++;
		
		printf("n %d  %d \n", f++, op);
		
		if(op >= h->opcode_base) { 
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










