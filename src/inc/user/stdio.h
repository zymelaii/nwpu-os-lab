#ifndef MINIOS_USER_STDIO_H
#define MINIOS_USER_STDIO_H

#include <stdio.h>

#define STDIN   0
#define STDOUT  1

// lib/user/stdio.c
int	printf(const char *fmt, ...);
int	vprintf(const char *fmt, va_list);
u8	getch(void);
u8	getchar(void);

void	fflush(void);


#endif /* MINIOS_USER_STDIO_H */