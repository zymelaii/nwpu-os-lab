#ifndef MINIOS_STDIO_H
#define MINIOS_STDIO_H

#include <type.h>
#include <stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* NULL */

// lib/kern/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

#endif /* MINIOS_STDIO_H */