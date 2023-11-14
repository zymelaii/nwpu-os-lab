#include <assert.h>
#include <fat32.h>
#include <mmu.h>
#include <string.h>
#include <x86.h>

#include <kern/keyboard.h>
#include <kern/fs.h>
#include <kern/stdio.h>
#include <kern/syscall.h>

/*
 * 内核读入函数，由于没有文件描述符概念，
 * 所以强制assert让fd为0，平时的读入流也默认是0的
 * 现在只有从键盘缓冲区获取字符的能力
 */
ssize_t
kern_read(int fd, void *buf, size_t count)
{
	assert(fd == 0);
	
	char *s = buf;

	for (size_t i = 0 ; i < count ; i++) {
		char c = read_keyboard_buf();
		if (c == -1)
			return i;
		*s++ = c;
	}

	return count;
}

ssize_t
do_read(int fd, void *buf, size_t count)
{
	assert(buf < (void *)KERNBASE);
	assert(buf + count < (void *)KERNBASE);
	return kern_read(fd, buf, count);
}

/*
 * 内核写入函数，由于没有文件描述符概念，
 * 所以强制assert让fd为1，平时的输出流也默认是1的
 * 现在只有输出字符到终端的能力
 */
ssize_t
kern_write(int fd, const void *buf, size_t count)
{
	assert(fd == 1);
	
	const char *s = buf;
	for (size_t i = 0 ; i < count ; i++)
		kprintf("%c", *s++);

	return count;
}

ssize_t
do_write(int fd, const void *buf, size_t count)
{
	assert(buf < (void *)KERNBASE);
	assert(buf + count < (void *)KERNBASE);
	return kern_write(fd, buf, count);
}

#define SECTSIZE	512

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

u32 fat_start_sec;
u32 data_start_sec;
u32 fat_now_sec;
struct BPB bpb;

/*
 * 获取下一个簇号
 */
static u32
get_next_clus(u32 current_clus)
{
	u32 sec = current_clus * 4 / SECTSIZE;
	u32 off = current_clus * 4 % SECTSIZE;

	static u32 buf[SECTSIZE / 4];
	if (fat_now_sec != fat_start_sec + sec) {
		readsect(buf, fat_start_sec + sec);
		fat_now_sec = fat_start_sec + sec;
	}
	return buf[off / 4];
}

/*
 * 读入簇号对应的数据区的所有数据
 * 读到dst开头的位置，返回读入的尾指针
 */
static void *
read_data_sec(void *dst, u32 current_clus)
{
	current_clus -= 2;
	current_clus *= bpb.BPB_SecPerClus;
	current_clus += data_start_sec;

	for (int i = 0 ; i < bpb.BPB_SecPerClus ; i++, dst += SECTSIZE)
		readsect(dst, current_clus + i);
	return dst;
}

/*
 * 根据filename读取文件到dst处
 * filename要求是短目录项名（11个字节）
 * dst推荐是3GB + 48MB
 */
void
read_file(const char *filename, void *dst)
{
	assert(strlen(filename) == 11);
	
	readsect(&bpb, 0);
	
	fat_start_sec = bpb.BPB_RsvdSecCnt;
	data_start_sec = fat_start_sec + bpb.BPB_FATSz32 * bpb.BPB_NumFATs;

	u32 root_clus = bpb.BPB_RootClus;
	u32 file_clus = 0;

	assert(bpb.BPB_BytsPerSec == SECTSIZE && bpb.BPB_SecPerClus == 8);

	static char buf[SECTSIZE * 8];
	// 遍历目录项获取文件起始簇号
	while (root_clus < 0x0FFFFFF8) {
		void *read_end = read_data_sec((void *)buf, root_clus);
		for (struct DirectoryEntry *p = (void *)buf 
					; (void *)p < read_end ; p++) {
			if (strncmp(p->DIR_Name, filename, 11) == 0) {
				assert(p->DIR_FileSize <= 16 * MB);
				file_clus = (u32)p->DIR_FstClusHI << 16 | 
						p->DIR_FstClusLO;
				break;
			}
		}
		if (file_clus != 0)
			break;
		root_clus = get_next_clus(root_clus);
	}

	if (file_clus == 0)
		panic("file can't found! filename: %s", filename);
	// 读入文件
	while (file_clus < 0x0FFFFFF8) {
		dst = read_data_sec(dst, file_clus);
		file_clus = get_next_clus(file_clus);
	}
}