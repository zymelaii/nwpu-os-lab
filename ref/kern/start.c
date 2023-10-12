#include "type.h"
#include "protect.h"
#include "string.h"
#include "terminal.h"

u8 gdt_ptr[6]; /* 0~15:Limit  16~47:Base */
DESCRIPTOR gdt[GDT_SIZE];

void cstart()
{
	/* 将 LOADER 中的 GDT 复制到新的 GDT 中 */
	memcpy(&gdt,					/* New GDT */
	       (void *)(*((u32 *)(&gdt_ptr[2]))),	/* Base  of Old GDT */
	       *((u16 *)(&gdt_ptr[0])) + 1		/* Limit of Old GDT */
	);
	/* gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt/lgdt 的参数。*/
	u16 *p_gdt_limit = (u16 *)(&gdt_ptr[0]);
	u32 *p_gdt_base = (u32 *)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base = (u32)&gdt;
	
	clear_screen();
	// 在终端的第0行依次打出N(黑底白字)，W(黑底蓝字)，P(白底蓝字)，U(白底黑字)
	kprintf(TERMINAL_POS(0, 0),
		"N%fW%bP%fU", LIGHT_BLUE, WHITE, BLACK);
	// 在终端的第1行依次输出白底绿紫相间的字符
	for (char *s = "even old new york once Amsterdam", 
					*st = s ; *s ; s += 4) {
		kprintf(TERMINAL_POS(1, s - st),
			"%b%f%c%f%c%f%c%f%c",
				WHITE, GREEN, *(s + 0),
				FUCHUSIA, *(s + 1), GREEN,
				*(s + 2), FUCHUSIA, *(s + 3));
	}
}
