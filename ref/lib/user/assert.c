#include <assert.h>

#include <user/stdio.h>

/*
 * 当发生不可挽回的错误时就打印错误信息并使进程死循环
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("\x1b[0m\x1b[91muser panic at %s:%d: ", file, line);
	vprintf(fmt, ap);
	printf("\n\x1b[0m");
	va_end(ap);

	fflush();
	// 休眠CPU核，直接罢工
	while(1)
		/* do nothing */;
}

/*
 * 很像panic，但是不会休眠进程，就是正常打印信息
 */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("\x1b[0m\x1b[93muser warning at %s:%d: ", file, line);
	vprintf(fmt, ap);
	printf("\n\x1b[0m");
	va_end(ap);

	fflush();
}