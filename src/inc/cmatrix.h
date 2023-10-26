#ifndef MINIOS_CMATRIX_H
#define MINIOS_CMATRIX_H

#include "type.h"
struct color_char {
	u8 foreground;//输出字符的前景色(0~15)
	u8 background;//输出字符的背景色(0~15)
	u8 print_char;//输出字符的ASCII码(0~127)
	u16 print_pos;//输出字符在终端中的位置
};

void cmatrix_start();

#endif /* MINIOS_CMATRIX_H */