#ifndef MINIOS_KERN_KEYBOARD_H
#define MINIOS_KERN_KEYBOARD_H

#include <type.h>

void	add_keyboard_buf(u8 ch);
u8	read_keyboard_buf(void);

#endif /* MINIOS_KERN_KEYBOARD_H */