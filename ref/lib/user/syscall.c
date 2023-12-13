#include <user/syscall.h>

ssize_t
syscall0(size_t NR_syscall)
{
	ssize_t ret;
	asm volatile("int $0x80"
		: "=a"(ret)
		: "a" (NR_syscall) 
		: "cc", "memory");
	return ret;
}

ssize_t
syscall1(size_t NR_syscall, size_t p1)
{
	ssize_t ret;
	asm volatile("int $0x80"
		: "=a"(ret)
		: "a" (NR_syscall),
		  "b" (p1)
		: "cc", "memory");
	return ret;
}
ssize_t 
syscall2(size_t NR_syscall, size_t p1, size_t p2)
{
	ssize_t ret;
	asm volatile("int $0x80"
		: "=a"(ret)
		: "a" (NR_syscall),
		  "b" (p1),
		  "c" (p2)
		: "cc", "memory");
	return ret;
}
ssize_t 
syscall3(size_t NR_syscall, size_t p1, size_t p2, size_t p3)
{
	ssize_t ret;
	asm volatile("int $0x80"
		: "=a"(ret)
		: "a" (NR_syscall),
		  "b" (p1),
		  "c" (p2),
		  "d" (p3)
		: "cc", "memory");
	return ret;
}
ssize_t 
syscall4(size_t NR_syscall, size_t p1, size_t p2, 
			size_t p3, size_t p4)
{
	ssize_t ret;
	asm volatile("int $0x80"
		: "=a"(ret)
		: "a" (NR_syscall),
		  "b" (p1),
		  "c" (p2),
		  "d" (p3),
		  "S" (p4)
		: "cc", "memory");
	return ret;
}
ssize_t 
syscall5(size_t NR_syscall, size_t p1, size_t p2, 
			size_t p3, size_t p4, size_t p5)
{
	ssize_t ret;
	asm volatile("int $0x80"
		: "=a"(ret)
		: "a" (NR_syscall),
		  "b" (p1),
		  "c" (p2),
		  "d" (p3),
		  "S" (p4),
		  "D" (p5)
		: "cc", "memory");
	return ret;
}

ssize_t
get_ticks(void)
{
	return syscall0(_NR_get_ticks);
}

ssize_t
get_pid(void)
{
	return syscall0(_NR_get_pid);
}

ssize_t
read(int fd, void *buf, size_t count)
{
	return syscall3(_NR_read, fd, (size_t)buf, count);
}

ssize_t
write(int fd, const void *buf, size_t count)
{
	return syscall3(_NR_write, fd, (size_t)buf, count);
}

ssize_t
exec(const char *pathname)
{
	return syscall1(_NR_exec, (size_t)pathname);
}

ssize_t
fork(void)
{
	return syscall0(_NR_fork);
}

ssize_t
exit(int status)
{
	return syscall1(_NR_exit, status);
}

ssize_t
wait(int *wstatus)
{
	return syscall1(_NR_wait, (size_t)wstatus);
}

ssize_t
fork_ack(void)
{
	return syscall0(_NR_fork_ack);
}