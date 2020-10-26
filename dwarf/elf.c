#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


#include "elf.h"
#include "dwarf.h"




void main(int argc, char* argv[]) {
	
	ELF elf = {0};
	struct stat st;

	elf.fd = open(argv[1], O_RDONLY);
	fstat(elf.fd, &st);
	elf.m = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, elf.fd, 0);
	
	elf.h1 = elf.m;
	elf.h2 = elf.m + sizeof(*elf.h1);
	
	// read the main header
	
	// check the magic bytes, 7F 45 4c 46
	if(elf.h1->magic[0] != 0x7f || elf.h1->magic[1] != 0x45 ||
		elf.h1->magic[2] != 0x4c || elf.h1->magic[3] != 0x46) {
		
		printf("not an ELF file\n");
	}
	else {
		printf("more magic\n");
	}
	
	// check address size/machine type
	if(elf.h1->class == 2) {
		printf("64 bit address size\n");
	}
	else if(elf.h1->class == 1) {
		printf("32 bit address size\n");
	}
	else {
		printf("unknown architecture\n");
	}
	
	// check endianness
	if(elf.h1->endianness == 1) {
		printf("little endian\n");
	}
	else if(elf.h1->endianness == 2) {
		printf("big endian\n");
	}
	else {
		printf("invalid endian byte\n");
	}
	
	// read the next header segment
	if(elf.h2->machine == 0x3E) {
		printf("amd64 architecture\n");
	}
	else {
		printf("unknown architecture\n");
	}
	
	// read the section header list
	elf.shl_cnt = elf.h2->section_h_num;
	elf.shl = elf.m + elf.h2->section_h_off;
	
	// section name pointers
	elf.names = elf.m + elf.shl[elf.h2->section_name_index].file_offset;

	
	
	for(int i = 0; i < elf.shl_cnt; i++) {
		char* name = elf.names + elf.shl[i].name;
		printf("%d: %s type: 0x%x, off: %d, sz: %d \n", i, 
			   name,
			   elf.shl[i].type, elf.shl[i].file_offset, elf.shl[i].size);
		
		if(0 == strcmp(name, ".debug_line")) {
			elf.dw_line = elf.m + elf.shl[i].file_offset;
			elf.dw_line_sz = elf.shl[i].size;
		}
		else if(0 == strcmp(name, ".debug_info")) {
			elf.dw_info = elf.m + elf.shl[i].file_offset;
			elf.dw_info_sz = elf.shl[i].size;
		}
	}
	
	
// 	line_num_machine(elf.dw_line, elf.dw_line_sz);
	
	debug_info_parse(elf.dw_info, elf.dw_info_sz);
}















