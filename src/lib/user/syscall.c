#include <user/syscall.h>

// 第一个冒号区间：汇编命令
// 第二个冒号区间：输出操作符扩展
// "=a" 这条命令结束后将eax寄存器的值赋给ret变量
// 第三个冒号区间：输入操作符扩展
// "a" 将变量赋给eax寄存器
// "b" 将变量赋给ebx寄存器
// "c" 将变量赋给ecx寄存器
// "d" 将变量赋给edx寄存器
// "S" 将变量赋给esi寄存器
// "D" 将变量赋给edi寄存器
// 第四个冒号区间：指令约束，当汇编指令会修改相关操作时需要修改
// 最主要是防被gcc编译器优化
// "a/b/c/d/S/D" 提醒编译器这些寄存器的值会在汇编指令执行中被修改
// "cc" 提醒编译器eflags寄存器的值在汇编指令执行中会被修改
// "memory" 提醒编译器内存中的数据在汇编指令执行中会被修改

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
ssize_t syscall2(size_t NR_syscall, size_t p1, size_t p2)
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
ssize_t syscall3(size_t NR_syscall, size_t p1, size_t p2, size_t p3)
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
ssize_t syscall4(size_t NR_syscall, size_t p1, size_t p2, 
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
ssize_t syscall5(size_t NR_syscall, size_t p1, size_t p2, 
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
get_ticks()
{
	return syscall0(_NR_get_ticks);
}

ssize_t
get_pid()
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