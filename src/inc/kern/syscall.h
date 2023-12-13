#ifndef MINIOS_KERN_SYSCALL_H
#define MINIOS_KERN_SYSCALL_H

#include <syscall.h>
#include <type.h>

// 由于会出现系统底层函数复用的情况
// miniOS 系统调用分成三层函数调用
// sys_*	外层函数，放进系统调用表中
// do_*		中间函数，留着用于参数处理
// kern_*	底层函数，实际处理函数

// 系统调用处理函数表
// kern/syscall.c
extern ssize_t (*syscall_table[])(void);

// kern/time.c
ssize_t	do_get_ticks(void);
// kern/process.c
ssize_t	do_get_pid(void);
// kern/exec.c
ssize_t	do_exec(const char *pathname);
// kern/exit.c
ssize_t	do_exit(int status);
// kern/fork.c
ssize_t	do_fork(void);
// kern/wait.c
ssize_t	do_wait(int *wstatus);
// kern/fs.c
ssize_t	do_read(int fd, void *buf, size_t count);
ssize_t	do_write(int fd, const void *buf, size_t count);
// 不告诉你这个实现在哪
ssize_t do_fork_ack(void);

#endif /* MINIOS_KERN_SYSCALL_H */