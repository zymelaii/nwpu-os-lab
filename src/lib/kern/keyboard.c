#include <type.h>
#include <x86.h>

#include <kern/keymap.h>
#include <kern/trap.h>

#define KB_INBUF_SIZE 1024

typedef struct kb_inbuf {
	u32	lock;
	u8*	p_head;
	u8*	p_tail;
	int	count;
	u8	buf[KB_INBUF_SIZE];
} KB_INPUT;

static KB_INPUT kb_input = {
	.p_head = kb_input.buf,
	.p_tail = kb_input.buf,
};

/*
 * 将ch这个字符放进内核的字符缓冲区
 */
void
add_keyboard_buf(u8 ch)
{
	// 上个自旋锁，保证线程安全（不建议用中断）计组课上讲过，相信大家的记忆力
	while (xchg(&kb_input.lock, 1) == 1);

	if (kb_input.count == KB_INBUF_SIZE)
		goto free;

	*kb_input.p_tail++ = ch;
	if (kb_input.p_tail == kb_input.buf + KB_INBUF_SIZE)
		kb_input.p_tail = kb_input.buf;
	kb_input.count++;

free:
	xchg(&kb_input.lock, 0);
}

/*
 * 如果内核的字符缓冲区为空，则返回-1
 * 否则返回缓冲区队头的字符并弹出队头
 */
u8
read_keyboard_buf(void)
{
	u8 res;

	// 上个自旋锁，保证线程安全（不建议用中断）计组课上讲过，相信大家的记忆力
	while (xchg(&kb_input.lock, 1) == 1);

	if (kb_input.count == 0) {
		res = -1;
		goto free;
	}
	
	res = *kb_input.p_head++;
	if (kb_input.p_head == kb_input.buf + KB_INBUF_SIZE) 
		kb_input.p_head = kb_input.buf;
	kb_input.count--;

free:
	xchg(&kb_input.lock, 0);
	return res;
}