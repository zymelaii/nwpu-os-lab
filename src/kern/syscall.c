#include <assert.h>

#include <kern/fs.h>
#include <kern/process.h>
#include <kern/syscall.h>
#include <kern/time.h>

static ssize_t sys_get_ticks(void);
static ssize_t sys_get_pid(void);
static ssize_t sys_read(void);
static ssize_t sys_write(void);
static ssize_t sys_exec(void);
static ssize_t sys_fork(void);
static ssize_t sys_wait(void);
static ssize_t sys_exit(void);
static ssize_t sys_fork_ack(void);

ssize_t (*syscall_table[])(void) = {
[_NR_get_ticks]	sys_get_ticks,
[_NR_get_pid]	sys_get_pid,
[_NR_read]	sys_read,
[_NR_write]	sys_write,
[_NR_exec]	sys_exec,
[_NR_fork]	sys_fork,
[_NR_wait]	sys_wait,
[_NR_exit]	sys_exit,
[_NR_fork_ack]	sys_fork_ack,
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

/*
 * ssize_t exec(const char *pathname)
 * 在进程中启动另一个程序执行的方法，加载为pathname对应的ELF文件
 */
static ssize_t
sys_exec(void)
{
	return do_exec((const void *)get_arg(0));
}

/*
 * ssize_t fork(void)
 * 复制一个一模一样的进程
 */
static ssize_t
sys_fork(void)
{
	return do_fork();
}

/*
 * ssize_t wait(int *wstatus)
 * 等待子进程exit，回收子进程资源，wstatus接收子进程的返回码
 */
static ssize_t
sys_wait(void)
{
	return do_wait((int *)get_arg(0));
}

/*
 * ssize_t exit(int status)
 * 进程退出的方法，将status作为返回码传递给父进程
 */
static ssize_t
sys_exit(void)
{
	return do_exit(get_arg(0));
}

/*
 * ssize_t fork_ack(void)
 * forkbomb.c所用，用于检测成功fork的次数
 * 内部实现被封装不公开
 */
static ssize_t
sys_fork_ack(void)
{
	return do_fork_ack();
}