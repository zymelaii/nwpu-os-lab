#ifndef MINIOS_KERN_STDIO_H
#define MINIOS_KERN_STDIO_H

#include <stdio.h>

// lib/terminal.c
// 正常输出函数，输出到屏幕中
int	kprintf(const char *fmt, ...);
int	vkprintf(const char *fmt, va_list);

// lib/serial.c
// 串口输出到终端中，而不是输出到屏幕中
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);

#endif /* MINIOS_KERN_STDIO_H */