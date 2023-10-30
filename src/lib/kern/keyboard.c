#include <stdio.h>
#include <type.h>

#include <kern/keymap.h>

#define KB_INBUF_SIZE 4

struct s_kb_inbuf {
	u8*	p_head;
	u8*	p_tail;
	int	count;
	u8	buf[KB_INBUF_SIZE];
};
typedef struct s_kb_inbuf kb_inbuf_t;

static kb_inbuf_t kb_input = {
	.p_head = kb_input.buf,
	.p_tail = kb_input.buf,
	.count = 0,
};

void
add_keyboard_buf(u8 ch)
{
	//! NOTE: assume that buf is available
	++kb_input.count;
	int next = (kb_input.p_tail - kb_input.buf + 1) % KB_INBUF_SIZE;
	*kb_input.p_tail = ch;
	kb_input.p_tail = &kb_input.buf[next];
	return;
}

u8
getch(void)
{
	if (kb_input.count == 0) {
		return 0xff;
	}
	int next = (kb_input.p_head - kb_input.buf + 1) % KB_INBUF_SIZE;
	u8 ch = *kb_input.p_head;
	--kb_input.count;
	kb_input.p_head = &kb_input.buf[next];
	return ch;
}
