#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <terminal.h>
#include <type.h>
#include <x86.h>

#include <kern/stdio.h>
#include <kern/trap.h>

inline static void 
write_to_terminal(u16 disp_pos, u16 content)
{
	asm(
	    "mov %1, %%gs:(%0)" ::"r"(disp_pos * 2), "r"(content)
	    : "memory");
}

struct kprintfbuf {
	// 通用
	u16	color;
	i16	cursor_row;
	i16	cursor_col;
	int	cnt;
	// 控制字符
	int	CSI;
	i16	param1;
	i16	param2;
};

#define CSI_ESC		0
#define CSI_BRACKET	1
#define CSI_PARAM1	2
#define CSI_PARAM2	3

static struct kprintfbuf TTY = {
	.color = DEFAULT_COLOR,
	.cursor_row = 1,
	.cursor_col = 1,
};

inline static u16
cursor_pos(struct kprintfbuf *b)
{
	return TERMINAL_POS(b->cursor_row, b->cursor_col);
}

static void
cursor_move(i16 move_row, i16 move_col, struct kprintfbuf *b)
{
	b->cursor_row += move_row;
	if (b->cursor_row < 1)
		b->cursor_row = 1;
	if (b->cursor_row > TERMINAL_ROW)
		b->cursor_row = TERMINAL_ROW;
	
	b->cursor_col += move_col;
	if (b->cursor_col < 1)
		b->cursor_col = 1;
	if (b->cursor_col > TERMINAL_COLUMN)
		b->cursor_col = TERMINAL_COLUMN;
}

#define CLEAR_CURSOR2END	0
#define CLEAR_CURSOR2BEGIN	1
#define CLEAR_ENTIRE		2

static void
clear_screen(struct kprintfbuf *b)
{
	u16 content = DEFAULT_COLOR | ' ';
	if (b->param1 == CLEAR_CURSOR2END) {
		u16 disp_pos = cursor_pos(b);
		while (disp_pos < TERMINAL_SIZE)
			write_to_terminal(disp_pos++, content);
	} else if (b->param1 == CLEAR_CURSOR2BEGIN) {
		u16 disp_pos = cursor_pos(b);
		while (disp_pos > 0)
			write_to_terminal(disp_pos--, content);
		write_to_terminal(disp_pos, content);
	} else if (b->param1 == CLEAR_ENTIRE) {
		u16 disp_pos = 0;
		while (disp_pos < TERMINAL_SIZE)
			write_to_terminal(disp_pos++, content);
	}
}

static void
clear_line(struct kprintfbuf *b)
{
	u16 content = DEFAULT_COLOR | ' ';
	if (b->param1 == CLEAR_CURSOR2END) {
		u16 disp_pos = cursor_pos(b);
		while (disp_pos % TERMINAL_COLUMN != 0)
			write_to_terminal(disp_pos++, content);
	} else if (b->param1 == CLEAR_CURSOR2BEGIN) {
		u16 disp_pos = cursor_pos(b);
		while (disp_pos % TERMINAL_COLUMN != 0)
			write_to_terminal(disp_pos--, content);
		write_to_terminal(disp_pos, content);
	} else if (b->param1 == CLEAR_ENTIRE) {
		u16 disp_pos = TERMINAL_POS(b->cursor_row, 1);
		do {
			write_to_terminal(disp_pos++, content);
		} while (disp_pos % TERMINAL_COLUMN != 0);
	}
}

static void
scroll(i16 scroll_up_num)
{
	scroll_up_num = MIN(scroll_up_num, +TERMINAL_ROW);
	scroll_up_num = MAX(scroll_up_num, -TERMINAL_ROW);

	if (scroll_up_num > 0) {
		void *dst = TERMINAL_POS(1, 1) * 2;
		dst = dst + TERMINAL_ADDR;
		void *src = (void*)(TERMINAL_POS(1 + scroll_up_num, 1) * 2);
		src = src + TERMINAL_ADDR;
		size_t clear_size = scroll_up_num * TERMINAL_COLUMN * 2;
		size_t copy_size = TERMINAL_SIZE * 2 - clear_size;

		memcpy(dst, src, copy_size);

		void *v = dst + copy_size;
		memset(v, 0, clear_size);
	} else if (scroll_up_num < 0) {
		i16 scroll_down_num = -scroll_up_num;
		void *dst = (void*)(TERMINAL_POS(1 + scroll_down_num, 1) * 2);
		dst = dst + TERMINAL_ADDR;
		void *src = TERMINAL_POS(1, 1) * 2;
		src = src + TERMINAL_ADDR;
		size_t clear_size = scroll_down_num * TERMINAL_COLUMN * 2;
		size_t copy_size = TERMINAL_SIZE * 2 - clear_size;

		memcpy(dst, src, copy_size);

		void *v = src;
		memset(v, 0, clear_size);
	}
}

inline static void
param12vga_color(struct kprintfbuf *b)
{
	u8 tmp = b->param1 & 1;
	b->param1 &= 0b0110;
	b->param1 |= b->param1 >> 2;
	b->param1 &= 0b0011;
	b->param1 |= tmp << 2;
}

static void
set_color(struct kprintfbuf *b)
{
	if (b->param1 == 0) {
		b->color = DEFAULT_COLOR;
	} else if (b->param1 == 1) {
		b->color |= 0x8800;
	} else if (b->param1 == 2) {
		b->color &= 0x7700;
	} else if (30 <= b->param1 && b->param1 <= 37) {
		b->param1 -= 30;
		param12vga_color(b);
		b->color = (b->color & 0xf8ff) | FOREGROUND(b->param1);
	} else if (40 <= b->param1 && b->param1 <= 47) {
		b->param1 -= 40;
		param12vga_color(b);
		b->color = (b->color & 0x8fff) | BACKGROUND(b->param1);
	} else if (90 <= b->param1 && b->param1 <= 97) {
		b->param1 -= 90;
		param12vga_color(b);
		b->param1 |= 0x8;
		b->color = (b->color & 0xf0ff) | FOREGROUND(b->param1);
	} else if (100 <= b->param1 && b->param1 <= 107) {
		b->param1 -= 100;
		param12vga_color(b);
		b->param1 |= 0x8;
		b->color = (b->color & 0x0fff) | BACKGROUND(b->param1);
	} else {
		warn("unsupport CSI: %dm", b->param1);
	}
}

static void
CSI_handler(u8 terminator, struct kprintfbuf *b)
{
	b->CSI = CSI_ESC;

	switch (terminator) {
	case 'A':
		if (b->param1 == 0)
			b->param1 = 1;
		cursor_move(-b->param1, 0, b);
		break;
	case 'B':
		if (b->param1 == 0)
			b->param1 = 1;
		cursor_move(+b->param1, 0, b);
		break;
	case 'C':
		if (b->param1 == 0)
			b->param1 = 1;
		cursor_move(0, +b->param1, b);
		break;
	case 'D':
		if (b->param1 == 0)
			b->param1 = 1;
		cursor_move(0, -b->param1, b);
		break;
	case 'E':
		if (b->param1 == 0)
			b->param1 = 1;
		cursor_move(+b->param1, 1 - b->cursor_col, b);
		break;
	case 'F':
		if (b->param1 == 0)
			b->param1 = 1;
		cursor_move(-b->param1, 1 - b->cursor_col, b);
		break;
	case 'H':
		if (b->param1 == 0)
			b->param1 = 1;
		if (b->param2 == 0)
			b->param2 = 1;
		cursor_move(b->param1 - b->cursor_row, 
				b->param2 - b->cursor_col, b);
		break;
	case 'J':
		clear_screen(b);
		break;
	case 'K':
		clear_line(b);
		break;
	case 'S':
		if (b->param1 == 0)
			b->param1 = 1;
		scroll(+b->param1);
		break;
	case 'T':
		if (b->param1 == 0)
			b->param1 = 1;
		scroll(-b->param1);
		break;
	case 'm':
		set_color(b);
		break;
	default:
		warn("unsupport CSI: %c", terminator);
		break;
	}
}

static void 
cursor_move_pre(struct kprintfbuf *b)
{
	if (b->cursor_col == 1) {
		if (b->cursor_row > 1) {
			b->cursor_row--;
		} else {
			scroll(-1);
		}
		b->cursor_col = TERMINAL_COLUMN;
	} else {
		b->cursor_col--;
	}
}

static void
cursor_move_nxt(struct kprintfbuf *b)
{
	if (b->cursor_col == TERMINAL_COLUMN) {
		if (b->cursor_row < TERMINAL_ROW) {
			b->cursor_row++;
		} else {
			scroll(1);
		}
		b->cursor_col = 1;
	} else {
		b->cursor_col++;
	}
}

static void 
kprintfputch(int ch, struct kprintfbuf *b)
{
	ch = ch & 0xff;
	b->cnt++;

	reswitch:
	if (b->CSI == CSI_ESC) {
		switch (ch) {
		case '\b':
			cursor_move_pre(b);
			break;
		case '\t':
			if (cursor_pos(b) == TERMINAL_SIZE - 1)
				break;
			while (b->cursor_col % 4 != 1)
				cursor_move_nxt(b);
			break;
		case '\n':
			cursor_move(0, TERMINAL_COLUMN - b->cursor_col, b);
			cursor_move_nxt(b);
			break;
		case '\x1b':
			b->CSI++;
			break;
		default:
		{
			u16 disp_pos = TERMINAL_POS(b->cursor_row, b->cursor_col);
			u16 content = b->color | ch;
			write_to_terminal(disp_pos, content);
			cursor_move_nxt(b);
			break;
		}
		}
	} else if (b->CSI == CSI_BRACKET) {
		switch (ch) {
		case '[':
			b->CSI++;
			b->param1 = b->param2 = 0;
			break;
		default:
			b->CSI = CSI_ESC;
			break;
		}
	} else {
		switch (ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (b->CSI == CSI_PARAM1) 
				b->param1 = b->param1 * 10 + ch - '0';
			else if (b->CSI == CSI_PARAM2)
				b->param2 = b->param2 * 10 + ch - '0';
			else
				;//do nothing
			break;
		case ';':
			b->CSI++;
			break;
		default:
			if (!(0x20 <= ch && ch <= 0x7e))
				b->CSI = CSI_ESC;
			if (0x40 <= ch && ch <= 0x7e)
				CSI_handler(ch, b);
			break;
		}
	}
}

int
vkprintf(const char *fmt, va_list ap)
{
	int rc;

	DISABLE_INT();
		TTY.cnt = 0;
		vprintfmt((void*)kprintfputch, &TTY, fmt, ap);
		rc = TTY.cnt;
	ENABLE_INT();

	return rc;
}

/*
 * 内核格式化输出函数，除了支持正常的%d,%s等格式化输出外
 * 还支持ANSI转义序列中的CSI sequence(以"ESC(十六进制字符为\x1b)["开头的序列，用于控制光标和终端)
 * 依据维基百科上的手册实现(en.wikipedia.org/wiki/ANSI_escape_code)
 * 目前支持的功能有：
 * \x1b[nA 将光标向上移动n(\x1b[A等价于\x1b[1A)行，如果移动中接触到边界则不继续移动
 * \x1b[nB 将光标向下移动n(\x1b[B等价于\x1b[1B)行，如果移动中接触到边界则不继续移动
 * \x1b[nC 将光标向右移动n(\x1b[C等价于\x1b[1C)列，如果移动中接触到边界则不继续移动
 * \x1b[nD 将光标向左移动n(\x1b[D等价于\x1b[1D)列，如果移动中接触到边界则不继续移动
 * \x1b[nE 将光标向下移动n(\x1b[E等价于\x1b[1E)行后移动到行首，如果移动中接触到边界则不继续移动
 * \x1b[nF 将光标向上移动n(\x1b[F等价于\x1b[1F)行后移动到行首，如果移动中接触到边界则不继续移动
 * \x1b[n;mH 将光标移动到n行m列（1行1列为终端左上角），n、m为空时默认为1，如果移动中接触到边界则不继续移动
 * 	可以不输出";"特殊情况举例：
 * 	\x1b[;5H等价于\x1b[1;5H
 * 	\x1b[5H等价于\x1b[5;H等价于\x1b[5;1H
 * 	\x1b[H等价于\x1b[1;1H
 * \x1b[nJ 终端清屏操作(\x1b[J等价于\x1b[0J)
 * 	当n为0时，清除当前光标位置到终端末尾的所有字符输出
 * 	当n为1时，清除当前光标位置到终端开头的所有字符输出
 * 	当n为2时，清除当前终端的所有字符输出
 * \x1b[nK 当前行清除字符操作(\x1b[J等价于\x1b[0J)
 * 	当n为0时，清除当前光标位置到行末的所有字符输出
 * 	当n为1时，清除当前光标位置到行首的所有字符输出
 * 	当n为2时，清除光标位置所在行的所有字符输出
 * \x1b[nS 将整个终端显示向上移动(\x1b[S等价于\x1b[1S)行，底部字符用空白填充
 * \x1b[nT 将整个终端显示向下移动(\x1b[T等价于\x1b[1T)行，顶部字符用空白填充
 * \x1b[nm 控制终端之后要输出的颜色(\x1b[m等价于\x1b[0m)：
 * 	当n为0时，将终端之后要输出的字符颜色置为默认色（白底黑字）
 * 	当n为1时，将终端之后要输出的字符颜色亮度提高（如果之前输出的是暗色）
 * 	当n为2时，将终端之后要输出的字符颜色亮度降低（如果之前输出的是亮色）
 * 	当n为30/40时，将终端之后要输出的字符的前景色/背景色置为黑色
 * 	当n为31/41时，将终端之后要输出的字符的前景色/背景色置为红色
 * 	当n为32/42时，将终端之后要输出的字符的前景色/背景色置为绿色
 * 	当n为33/43时，将终端之后要输出的字符的前景色/背景色置为棕色
 * 	当n为34/44时，将终端之后要输出的字符的前景色/背景色置为蓝色
 * 	当n为35/45时，将终端之后要输出的字符的前景色/背景色置为洋红色
 * 	当n为36/46时，将终端之后要输出的字符的前景色/背景色置为青色
 * 	当n为37/47时，将终端之后要输出的字符的前景色/背景色置为银色
 * 	当n为90/100时，将终端之后要输出的字符的前景色/背景色置为灰色
 * 	当n为91/101时，将终端之后要输出的字符的前景色/背景色置为亮红色
 * 	当n为92/102时，将终端之后要输出的字符的前景色/背景色置为亮绿色
 * 	当n为93/103时，将终端之后要输出的字符的前景色/背景色置为黄色
 * 	当n为94/104时，将终端之后要输出的字符的前景色/背景色置为亮蓝色
 * 	当n为95/105时，将终端之后要输出的字符的前景色/背景色置为亮洋红色
 * 	当n为96/106时，将终端之后要输出的字符的前景色/背景色置为亮青色
 * 	当n为97/107时，将终端之后要输出的字符的前景色/背景色置为白色色
 */
int
kprintf(const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vkprintf(fmt, ap);
	va_end(ap);

	return rc;
}