#include <assert.h>
#include <elf.h>
#include <fat32.h>
#include <string.h>
#include <x86.h>

#include <kern/stdio.h>

/*
 * 当发生不可挽回的错误时就打印错误信息并使CPU核休眠
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	// 确保CPU核不受外界中断的影响
	asm volatile("cli");
	asm volatile("cld");

	va_start(ap, fmt);
	kprintf("\x1b[0m\x1b[91mkernel panic at %s:%d: ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n\x1b[0m");
	va_end(ap);
	// 休眠CPU核，直接罢工
	while(1)
		asm volatile("hlt");
}

/*
 * 很像panic，但是不会休眠CPU核，就是正常打印信息
 */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf("\x1b[0m\x1b[93mkernel warning at %s:%d: ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n\x1b[0m");
	va_end(ap);
}

#define SECTSIZE	512
#define CLUSIZE		4096
#define FAT_ADDR	0x30000
#define ELF_ADDR	0x40000
#define BUF_ADDR	0x50000
#define KERNEL_NAME	"KERNEL  BIN"

u32 fat_start_sec;
u32 data_start_sec;
u32 elf_clus;
u32 elf_off;
u32 fat_now_sec;
struct BPB bpb;

static void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

static void
readsect(void *dst, u32 offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}

static u32
get_next_clus(u32 current_clus)
{
	u32 sec = current_clus * 4 / SECTSIZE;
	u32 off = current_clus * 4 % SECTSIZE;
	if (fat_now_sec != fat_start_sec + sec) {
		readsect((void *)FAT_ADDR, fat_start_sec + sec);
		fat_now_sec = fat_start_sec + sec;
	}
	return *(u32 *)(FAT_ADDR + off);
}

/*
 * 读入簇号对应的数据区的所有数据
 */
static void *
read_data_sec(void *dst, u32 clus)
{
	u32 sec = (clus - 2) * bpb.BPB_SecPerClus;
	sec += data_start_sec;

	for (int i = 0 ; i < bpb.BPB_SecPerClus ; i++, dst += SECTSIZE)
		readsect(dst, sec + i);
	return dst;
}

/*
 * 根据输入的参数读取对应的一段，由于kernel.bin是正经ld链接的，所以文件偏移和
 * 虚拟地址的增长速度相同，所以采取激进的读取策略，如果文件偏移
 * 不能从当前的elf_clus推出到需要读取的簇号则直接默认为读取失败，
 */
static void
readseg(void *va, u32 count, u32 offset)
{
	void *end_va = va + count;

	assert(offset >= elf_off);
	
	while (va < end_va) {
		// 如果[elf_off, elf_off + CLUSIZE)里面有段需要的数据
		// 就将对应的数据从文件中加载到指定位置，否则找下一个簇，移动elf_off的位置
		// 如果要加载整个段，就直接读入
		// 如果要加载部分段，就将其读入至缓存再从缓存转移到指定位置
		if (offset - elf_off < CLUSIZE) {
			u32 buf_off = offset - elf_off;
			u32 buf_len = MIN(CLUSIZE - buf_off, end_va - va);

			if (buf_off == 0 && buf_len == CLUSIZE) {
				read_data_sec((void *)va, elf_clus);
			} else {
				read_data_sec((void *)BUF_ADDR, elf_clus);
				memcpy( va,
					(void *)BUF_ADDR + buf_off,
					buf_len);
			}

			va += buf_len;
			offset += buf_len;
		} else {
			elf_off += CLUSIZE;
			elf_clus = get_next_clus(elf_clus);
		}
	}
}

static u32
find_kernel_elf_clus(void)
{
	u32 elf_clus = 0;
	u32 root_clus = bpb.BPB_RootClus;
	
	while (root_clus < 0x0FFFFFF8) {
		struct DirectoryEntry *p = (void *)BUF_ADDR;
		void *buf_end = read_data_sec((void *)BUF_ADDR, root_clus);
		for (; p < (struct DirectoryEntry *)buf_end ; p++) {
			// 排除长目录项
			if (p->DIR_Attr == 0xf)
				continue;
			if (strncmp(p->DIR_Name, KERNEL_NAME, 11) == 0) {
				elf_clus = (u32)p->DIR_FstClusHI << 16 | 
						p->DIR_FstClusLO;
				break;
			}
		}
		if (elf_clus != 0)
			break;
		root_clus = get_next_clus(root_clus);
	}
	assert(elf_clus);

	return elf_clus;
}

static void
load_kernel_elf(Elfhdr_t *eh)
{
	Proghdr_t *ph = (void *)eh + eh->e_phoff;
	for (int i = 0 ; i < eh->e_phnum ; i++, ph++) {
		if (ph->p_type != PT_LOAD)
			continue;
		readseg((void *)ph->p_va, ph->p_filesz, ph->p_offset);
		memset((void *)ph->p_va + ph->p_filesz, 
			0, 
			ph->p_memsz - ph->p_filesz);
	}
}

/*
 * 初始化函数，加载kernel.bin的elf文件并跳过去。
 */
void 
load_kernel(void)
{
	kprintf("\x1b[2J\x1b[H");
	kprintf("----start loading kernel elf----\n");
	
	// 获取文件系统引导头
	readsect((void *)&bpb, 0);
	assert(bpb.BPB_BytsPerSec == SECTSIZE && bpb.BPB_SecPerClus == 8);

	fat_start_sec = bpb.BPB_RsvdSecCnt;
	data_start_sec = fat_start_sec + bpb.BPB_FATSz32 * bpb.BPB_NumFATs;

	// 找kernel文件的首簇号
	elf_clus = find_kernel_elf_clus();

	// 读取elf头
	read_data_sec((void *)ELF_ADDR, elf_clus);
	Elfhdr_t *eh = (void *)ELF_ADDR;
	assert(eh->e_ehsize <= CLUSIZE);
	
	// 将elf的内容加载到指定位置
	load_kernel_elf(eh);
	
	kprintf("----finish loading kernel elf----\n");
	((void (*)(void))(eh->e_entry))();
}