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
	return;
}

u8
getch(void)
{
	return 0;	
}