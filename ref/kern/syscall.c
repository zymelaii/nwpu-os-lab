#include <assert.h>

#include <kern/fs.h>
#include <kern/process.h>
#include <kern/syscall.h>
#include <kern/time.h>

static ssize_t sys_get_ticks(void);
static ssize_t sys_get_pid(void);
static ssize_t sys_read(void);
static ssize_t sys_write(void);

ssize_t (*syscall_table[])(void) = {
[_NR_get_ticks]	sys_get_ticks,
[_NR_get_pid]	sys_get_pid,
[_NR_read]	sys_read,
[_NR_write]	sys_write,
};

/*
 * 获取系统调用参数
 * 系统调用最多会使用6个参数
 * 	0	1	2	3	4	5
 * 	ebx	ecx	edx	esi	edi	ebp
 */
static u32
get_arg(int order)
{
	switch (order) {
	case 0:
		return p_proc_ready->pcb.user_regs.ebx;
	case 1:
		return p_proc_ready->pcb.user_regs.ecx;
	case 2:
		return p_proc_ready->pcb.user_regs.edx;
	case 3:
		return p_proc_ready->pcb.user_regs.esi;
	case 4:
		return p_proc_ready->pcb.user_regs.edi;
	case 5:
		return p_proc_ready->pcb.user_regs.ebp;
	default:
		panic("invalid order! order: %d", order);
	}
}
/*
 * ssize_t get_ticks(void)
 * 获取当前的时间戳
 */
static ssize_t 
sys_get_ticks(void)
{
	return do_get_ticks();
}

/*
 * int get_pid(void)
 * 获取当前进程的进程号
 */
static ssize_t
sys_get_pid(void)
{
	return do_get_pid();
}

/*
 * ssize_t read(int fd, void *buf, size_t count)
 * 目前在这个实验中这个系统调用的作用是从键盘中得到输入
 * 写入用户进程中以buf为开头的长度为count字节的缓冲区，返回值为写入的字节数
 */
static ssize_t
sys_read(void)
{
	return do_read(get_arg(0), (void *)get_arg(1), get_arg(2));
}

/*
 * ssize_t write(int fd, const void *buf, size_t count)
 * 目前在这个实验中这个系统调用的作用是往终端输出字符
 * 输出用户进程中以buf为开头的长度为count字节的缓冲区，返回值为写出去的字节数
 */
static ssize_t
sys_write(void)
{
	return do_write(get_arg(0), (const void *)get_arg(1), get_arg(2));
}