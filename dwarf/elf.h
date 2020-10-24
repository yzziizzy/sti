

struct ELF_h1 {
	uint8_t magic[4]; // 7F 45 4c 46
	uint8_t class; // 1 = 32bit, 2 = 64bit
	uint8_t endianness; // 1 = little, 2 = dumb
	uint8_t version;
	uint8_t os_abi;
	uint8_t abi_version;
	uint8_t pad[7];
} __attribute__ ((packed));

// fields from here on are affected by endianness.
struct ELF_h2_64 {
	uint16_t type;
	uint16_t machine; // x64 = 0x3E
	uint32_t version;
	uint64_t entry;
	uint64_t program_h_off;
	uint64_t section_h_off;
	uint32_t flags;
	uint16_t hsz;
	uint16_t program_h_sz;
	uint16_t program_h_num;
	uint16_t section_h_entry_sz;
	uint16_t section_h_num;
	uint16_t section_name_index;
} __attribute__ ((packed));

typedef struct ELF_sh_64 {
	uint32_t name;
	uint32_t type;
	uint64_t flags;
	uint64_t loaded_vma;
	uint64_t file_offset;
	uint64_t size;
	uint32_t link;
	uint32_t info;
	uint64_t addr_align;
	uint64_t entry_sz;
}  __attribute__ ((packed)) ELF_sh_64;


typedef struct ELF {
	int fd;
	void* m;
	
	struct ELF_h1* h1;
	struct ELF_h2_64* h2;
	
	int shl_cnt;
	ELF_sh_64* shl; // section header list
	char* names;
	
	void* dw_info;
	size_t dw_info_sz;
	
	void* dw_line;
	size_t dw_line_sz;
	
} ELF;
