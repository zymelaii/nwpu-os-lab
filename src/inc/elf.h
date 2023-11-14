#ifndef MINIOS_ELF_H
#define MINIOS_ELF_H

#include <type.h>

#define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */

struct s_Elfhdr {
	u32 e_magic;	// must equal ELF_MAGIC
	u8 e_elf[12];
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u32 e_entry;
	u32 e_phoff;
	u32 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;
};
typedef struct s_Elfhdr Elfhdr_t;

struct s_Proghdr {
	u32 p_type;
	u32 p_offset;
	u32 p_va;
	u32 p_pa;
	u32 p_filesz;
	u32 p_memsz;
	u32 p_flags;
	u32 p_align;
};
typedef struct s_Proghdr Proghdr_t;

struct s_Secthdr {
	u32 sh_name;
	u32 sh_type;
	u32 sh_flags;
	u32 sh_addr;
	u32 sh_offset;
	u32 sh_size;
	u32 sh_link;
	u32 sh_info;
	u32 sh_addralign;
	u32 sh_entsize;
};
typedef struct s_Secthdr Secthdr_t;

// Values for Proghdr_t::p_type
#define PT_LOAD			1

// Flag bits for Proghdr_t::p_flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE	2
#define ELF_PROG_FLAG_READ	4

// Values for Secthdr_t::sh_type
#define ELF_SHT_NULL		0
#define ELF_SHT_PROGBITS	1
#define ELF_SHT_SYMTAB		2
#define ELF_SHT_STRTAB		3

// Values for Secthdr_t::sh_name
#define ELF_SHN_UNDEF		0

#endif /* MINIOS_ELF_H */