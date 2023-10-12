#include "type.h"
#include "string.h"

// elf文件格式会用
typedef u32		Elf32_Addr;
typedef u16		Elf32_Half;
typedef u32		Elf32_Off;
typedef i32		Elf32_Sword;
typedef u32		Elf32_Word;

/*
 * elf相关
 */
#define KERNEL_ELF 0x80400

#define EI_NIDENT 16
typedef struct elf32_hdr {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry; /* Entry point */
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

typedef struct elf32_phdr {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;

/*
 * 显示相关
 */
#define TERMINAL_COLUMN	80
#define TERMINAL_ROW	25

#define TERMINAL_POS(row, column) ((u16)(row) * TERMINAL_COLUMN + (column))
/*
 * 终端默认色，黑底白字
 */
#define DEFAULT_COLOR 0x0f00

/*
 * 这个函数将content数据（2字节）写到终端第disp_pos个字符
 * 第0个字符在0行0列，第1个字符在0行1列，第80个字符在1行0列，以此类推
 * 用的是内联汇编，等价于mov word [gs:disp_pos * 2], content
 */
inline static void 
write_to_terminal(u16 disp_pos, u16 content)
{
	asm(
	    "mov %1, %%gs:(%0)" ::"r"(disp_pos * 2), "r"(content)
	    : "memory");
}

/*
 * 清屏
 */
static void
clear_screen()
{
	for (int i = 0; i < TERMINAL_ROW; i++)
		for (int j = 0; j < TERMINAL_COLUMN; j++)
			write_to_terminal(TERMINAL_POS(i, j), 
							DEFAULT_COLOR | ' ');
}

/*
 * 初始化函数，加载kernel.bin的elf文件并跳过去。
 * 不过幸运的是，你在本次实验中不需要关心这些内容，暂时把它当成BOOT里面加载LOADER这样的过程就好了。
 */
void 
load_kernel()
{
	clear_screen();
	for (char *s = "----start loading kernel elf----", *st = s; *s; s++)
		write_to_terminal(s - st, DEFAULT_COLOR | *s);

	Elf32_Ehdr *kernel_ehdr = (Elf32_Ehdr *)KERNEL_ELF;

	Elf32_Phdr *kernel_phdr = (void *)kernel_ehdr + kernel_ehdr->e_phoff;
	for (u32 i = 0; i < kernel_ehdr->e_phnum; i++, kernel_phdr++)
	{
		if (kernel_phdr->p_type != PT_LOAD)
			continue;
		// 将elf的文件数据复制到指定位置
		memcpy(
		    (void *)kernel_phdr->p_vaddr,
		    (void *)kernel_ehdr + kernel_phdr->p_offset,
		    kernel_phdr->p_filesz);
		// 将后面的字节清零(p_memsz >= p_filesz)
		memset(
		    (void *)kernel_phdr->p_vaddr + kernel_phdr->p_filesz,
		    0,
		    kernel_phdr->p_memsz - kernel_phdr->p_filesz);
	}
	((void (*)(void))(kernel_ehdr->e_entry))();
}