#include <type.h>
#include <string.h>
#include <elf.h>
#include <assert.h>

static const char *eh_lookup_ident_class(int i) {
	return (const char*[]){"INVALID", "32-BIT", "64-BIT"}[i];
}

static const char *eh_lookup_ident_encoding(int i) {
	return (const char*[]){"INVALID", "LSB", "MSB"}[i];
}

static const char *eh_lookup_type(int i) {
	switch (i) {
		case 0x0000: return "NONE";
		case 0x0001: return "REL";
		case 0x0002: return "EXEC";
		case 0x0003: return "DYN";
		case 0x0004: return "CORE";
		case 0xFE00: return "LOOS";
		case 0xFEFF: return "HIOS";
		case 0xFF00: return "LOPROC";
		case 0xFFFF: return "HIPROC";
		default: return "INVALID";
	}
}

static const char *eh_lookup_machine(int i) {
	switch (i) {
		case 0x00: return "None";
		case 0x01: return "AT&T WE 32100";
		case 0x02: return "SPARC";
		case 0x03: return "x86";
		case 0x04: return "Motorola 68000 (M68k)";
		case 0x05: return "Motorola 88000 (M88k)";
		case 0x06: return "Intel MCU";
		case 0x07: return "Intel 80860";
		case 0x08: return "MIPS";
		case 0x09: return "IBM System/370";
		case 0x0A: return "MIPS RS3000 Little-endian";
		case 0x0F: return "Hewlett-Packard PA-RISC";
		case 0x13: return "Intel 80960";
		case 0x14: return "PowerPC";
		case 0x15: return "PowerPC (64-bit)";
		case 0x16: return "S390, including S390x";
		case 0x17: return "IBM SPU/SPC";
		case 0x24: return "NEC V800";
		case 0x25: return "Fujitsu FR20";
		case 0x26: return "TRW RH-32";
		case 0x27: return "Motorola RCE";
		case 0x28: return "Arm (up to Armv7/AArch32)";
		case 0x29: return "Digital Alpha";
		case 0x2A: return "SuperH";
		case 0x2B: return "SPARC Version 9";
		case 0x2C: return "Siemens TriCore embedded processor";
		case 0x2D: return "Argonaut RISC Core";
		case 0x2E: return "Hitachi H8/300";
		case 0x2F: return "Hitachi H8/300H";
		case 0x30: return "Hitachi H8S";
		case 0x31: return "Hitachi H8/500";
		case 0x32: return "IA-64";
		case 0x33: return "Stanford MIPS-X";
		case 0x34: return "Motorola ColdFire";
		case 0x35: return "Motorola M68HC12";
		case 0x36: return "Fujitsu MMA Multimedia Accelerator";
		case 0x37: return "Siemens PCP";
		case 0x38: return "Sony nCPU embedded RISC processor";
		case 0x39: return "Denso NDR1 microprocessor";
		case 0x3A: return "Motorola Star*Core processor";
		case 0x3B: return "Toyota ME16 processor";
		case 0x3C: return "STMicroelectronics ST100 processor";
		case 0x3D: return "Advanced Logic Corp. TinyJ embedded processor family";
		case 0x3E: return "AMD x86-64";
		case 0x3F: return "Sony DSP Processor";
		case 0x40: return "Digital Equipment Corp. PDP-10";
		case 0x41: return "Digital Equipment Corp. PDP-11";
		case 0x42: return "Siemens FX66 microcontroller";
		case 0x43: return "STMicroelectronics ST9+ 8/16 bit microcontroller";
		case 0x44: return "STMicroelectronics ST7 8-bit microcontroller";
		case 0x45: return "Motorola MC68HC16 Microcontroller";
		case 0x46: return "Motorola MC68HC11 Microcontroller";
		case 0x47: return "Motorola MC68HC08 Microcontroller";
		case 0x48: return "Motorola MC68HC05 Microcontroller";
		case 0x49: return "Silicon Graphics SVx";
		case 0x4A: return "STMicroelectronics ST19 8-bit microcontroller";
		case 0x4B: return "Digital VAX";
		case 0x4C: return "Axis Communications 32-bit embedded processor";
		case 0x4D: return "Infineon Technologies 32-bit embedded processor";
		case 0x4E: return "Element 14 64-bit DSP Processor";
		case 0x4F: return "LSI Logic 16-bit DSP Processor";
		case 0x8C: return "TMS320C6000 Family";
		case 0xAF: return "MCST Elbrus e2k";
		case 0xB7: return "Arm 64-bits (Armv8/AArch64)";
		case 0xDC: return "Zilog Z80";
		case 0xF3: return "RISC-V";
		case 0xF7: return "Berkeley Packet Filter";
		default: return "Unknown";
	}
}

static const char *ep_lookup_type(int i) {
	switch (i) {
		case 0x00000000: return "PT_NULL";
		case 0x00000001: return "PT_LOAD";
		case 0x00000002: return "PT_DYNAMIC";
		case 0x00000003: return "PT_INTERP";
		case 0x00000004: return "PT_NOTE";
		case 0x00000005: return "PT_SHLIB";
		case 0x00000006: return "PT_PHDR";
		case 0x00000007: return "PT_TLS";
		case 0x60000000: return "PT_LOOS";
		case 0x6FFFFFFF: return "PT_HIO";
		case 0x70000000: return "PT_LOPROC";
		case 0x7FFFFFFF: return "PT_HIPRO";
		default: return "UNKNOWN";
	}
}

static const char *ep_lookup_flags(int i) {
	switch (i) {
		case 0b001: return "PF_X";
		case 0b010: return "PF_W";
		case 0b100: return "PF_R";
		case 0b011: return "PF_W | PF_X";
		case 0b101: return "PF_R | PF_X";
		case 0b110: return "PF_R | PF_W";
		case 0b111: return "PF_R | PF_W | PF_X";
		default: return "INVALID";
	}
}

static void dump_program_header(writefmt_t writefmt, Elfhdr_t *elf_header, int index) {
	Proghdr_t *prog = (void*)elf_header + elf_header->e_phoff + index * elf_header->e_phentsize;
	writefmt(
		"elf program header {\n"
		"  type: 0x%x \"%s\"\n"
		"  offset: %#p\n"
		"  virtual address: %#p\n"
		"  physical address: %#p\n"
		"  file size: %d\n"
		"  memory size: %d\n"
		"  flags: 0x%x \"%s\"\n"
		"  align: %d\n"
		"}\n",
		prog->p_type, ep_lookup_type(prog->p_type),
		prog->p_offset,
		prog->p_va,
		prog->p_pa,
		prog->p_filesz,
		prog->p_memsz,
		prog->p_flags, ep_lookup_flags(prog->p_flags),
		prog->p_align
	);
}

static void dump_section_header(writefmt_t writefmt, Elfhdr_t *elf_header, int index) {
	//! TODO: dump section header
}

void dump_elf_header(writefmt_t writefmt, Elfhdr_t *elf_header) {
	writefmt(
		"elf header {\n"
		"  ident {\n"
		"    magic: 0x%8x\n"
		"    class: %d \"%s\"\n"
		"    encoding: %d \"%s\"\n"
		"    version: %d\n"
		"  }\n"
		"  type: 0x%x \"%s\"\n"
		"  machine: 0x%x \"%s\"\n"
		"  version: %d\n"
		"  entry: %#p\n"
		"  program header offset: %d\n"
		"  section header offset: %d\n"
		"  flags: %d\n"
		"  elf header size: %d\n"
		"  program header size: %d\n"
		"  program header number: %d\n"
		"  section header size: %d\n"
		"  section header number: %d\n"
		"  section header index to names: %d\n"
		"}\n",
		elf_header->e_magic,
		elf_header->e_elf[0], eh_lookup_ident_class(elf_header->e_elf[0]),
		elf_header->e_elf[1], eh_lookup_ident_encoding(elf_header->e_elf[1]),
		elf_header->e_elf[2],
		elf_header->e_type, eh_lookup_type(elf_header->e_type),
		elf_header->e_machine, eh_lookup_machine(elf_header->e_machine),
		elf_header->e_version,
		elf_header->e_entry,
		elf_header->e_phoff,
		elf_header->e_shoff,
		elf_header->e_flags,
		elf_header->e_ehsize,
		elf_header->e_phentsize,
		elf_header->e_phnum,
		elf_header->e_shentsize,
		elf_header->e_shnum,
		elf_header->e_shstrndx
	);

	for (int i = 0; i < elf_header->e_phnum; ++i) {
		dump_program_header(writefmt, elf_header, i);
	}

	for (int i = 0; i < elf_header->e_shnum; ++i) {
		dump_section_header(writefmt, elf_header, i);
	}
}

void parse_elf_file(Elfhdr_t *elf_header) {
	//! TODO: parse elf file
}
