#include <x86.h>

#include <user/stdio.h>
#include <user/syscall.h>

#define PRINTFBUF_SIZE 4096

struct printfbuf {
	u32 lock;
	char buf[PRINTFBUF_SIZE];
	char *buf_p;
	int cnt;
};

static struct printfbuf printfb = {
	.buf_p = printfb.buf
};

static void
printfputch(int ch, struct printfbuf *b)
{
	b->cnt++;
	*b->buf_p++ = (char)ch;
	if (ch == '\n' || b->buf_p == b->buf + PRINTFBUF_SIZE) {
		write(STDOUT, b->buf, b->buf_p - b->buf);
		b->buf_p = b->buf;
	}
}

int
vprintf(const char *fmt, va_list ap)
{
	struct printfbuf *b = &printfb;

	// 上个自旋锁，保证线程安全（不建议用中断）计组课上讲过，相信大家的记忆力
	while (xchg(&b->lock, 1) == 1);

	b->cnt = 0;
	vprintfmt((void *)printfputch, b, fmt, ap);
	
	int rc = b->cnt;
	xchg(&b->lock, 0);

	return rc;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vprintf(fmt, ap);
	va_end(ap);

	return rc;
}

/*
 * 将printf的缓冲区全写出去
 */
void
fflush()
{
	struct printfbuf *b = &printfb;
	write(STDOUT, b->buf, b->buf_p - b->buf);
	b->buf_p = b->buf;
}


#define GETCHBUG_SIZE	1024
struct getchbuf {
	u32 lock;
	u8 buf[GETCHBUG_SIZE];
	u8 *st, *en;
};

static struct getchbuf getchb = {
	.st = getchb.buf,
	.en = getchb.buf,
};

u8
getch()
{
	struct getchbuf *b = &getchb;

	// 上个自旋锁，保证线程安全（不建议用中断）计组课上讲过，相信大家的记忆力
	while (xchg(&b->lock, 1) == 1);

	if (b->st == b->en) {
		b->st = b->en = b->buf;
		b->en += read(STDIN, b->buf, sizeof(b->buf));
	}
	
	u8 rc = b->st == b->en ? -1 : *b->st++;

	xchg(&b->lock, 0);

	return rc;
}