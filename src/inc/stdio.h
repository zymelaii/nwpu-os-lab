#ifndef MINIOS_STDIO_H
#define MINIOS_STDIO_H

#include <type.h>
#include <stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* NULL */

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/terminal.c
// 正常输出函数，输出到屏幕中
int	kprintf(const char *fmt, ...);
int	vkprintf(const char *fmt, va_list);

// lib/serial.c
// 串口输出到终端中，而不是输出到屏幕中
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);

// kern/keyboard.c
u8	getch(void);

#endif /* MINIOS_STDIO_H */