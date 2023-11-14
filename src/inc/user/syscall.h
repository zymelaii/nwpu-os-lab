#ifndef MINIOS_USER_SYSCALL_H
#define MINIOS_USER_SYSCALL_H

#include <type.h>
#include <syscall.h>

ssize_t syscall0(size_t NR_syscall);
ssize_t syscall1(size_t NR_syscall, size_t p1);
ssize_t syscall2(size_t NR_syscall, size_t p1, size_t p2);
ssize_t syscall3(size_t NR_syscall, size_t p1, size_t p2, size_t p3);
ssize_t syscall4(size_t NR_syscall, size_t p1, size_t p2, size_t p3, size_t p4);
ssize_t syscall5(size_t NR_syscall, size_t p1, size_t p2, size_t p3, size_t p4, size_t p5);

ssize_t get_ticks();
ssize_t get_pid();
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

#endif