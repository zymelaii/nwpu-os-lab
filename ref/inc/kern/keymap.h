#ifndef MINIOS_KERN_KEYMAP_H
#define MINIOS_KERN_KEYMAP_H

#include <type.h>

#define NR_SCAN_CODES 128

#define FLAG_BREAK	0x0080		/* Break Code			*/
#define FLAG_EXT	0x0100		/* Normal function keys		*/
#define FLAG_SHIFT_L	0x0200		/* Shift key			*/
#define FLAG_SHIFT_R	0x0400		/* Shift key			*/
#define FLAG_CTRL_L	0x0800		/* Control key			*/
#define FLAG_CTRL_R	0x1000		/* Control key			*/
#define FLAG_ALT_L	0x2000		/* Alternate key		*/
#define FLAG_ALT_R	0x4000		/* Alternate key		*/
#define FLAG_PAD	0x8000		/* keys in num pad		*/

#define MASK_RAW	0x01FF	/* raw key value = code_passed_to_tty & MASK_RAW
				 * the value can be found either in the keymap
				 * column 0 or in the list below
				 */

/* Special keys */
#define ESC		(0x01 + FLAG_EXT)	/* Esc		*/
#define TAB		(0x02 + FLAG_EXT)	/* Tab		*/
#define ENTER		(0x03 + FLAG_EXT)	/* Enter	*/
#define BACKSPACE	(0x04 + FLAG_EXT)	/* BackSpace	*/

#define GUI_L		(0x05 + FLAG_EXT)	/* L GUI	*/
#define GUI_R		(0x06 + FLAG_EXT)	/* R GUI	*/
#define APPS		(0x07 + FLAG_EXT)	/* APPS	*/

/* Shift, Ctrl, Alt */
#define SHIFT_L		(0x08 + FLAG_EXT)	/* L Shift	*/
#define SHIFT_R		(0x09 + FLAG_EXT)	/* R Shift	*/
#define CTRL_L		(0x0A + FLAG_EXT)	/* L Ctrl	*/
#define CTRL_R		(0x0B + FLAG_EXT)	/* R Ctrl	*/
#define ALT_L		(0x0C + FLAG_EXT)	/* L Alt	*/
#define ALT_R		(0x0D + FLAG_EXT)	/* R Alt	*/

/* Lock keys */
#define CAPS_LOCK	(0x0E + FLAG_EXT)	/* Caps Lock	*/
#define	NUM_LOCK	(0x0F + FLAG_EXT)	/* Number Lock	*/
#define SCROLL_LOCK	(0x10 + FLAG_EXT)	/* Scroll Lock	*/

/* Function keys */
#define F1		(0x11 + FLAG_EXT)	/* F1		*/
#define F2		(0x12 + FLAG_EXT)	/* F2		*/
#define F3		(0x13 + FLAG_EXT)	/* F3		*/
#define F4		(0x14 + FLAG_EXT)	/* F4		*/
#define F5		(0x15 + FLAG_EXT)	/* F5		*/
#define F6		(0x16 + FLAG_EXT)	/* F6		*/
#define F7		(0x17 + FLAG_EXT)	/* F7		*/
#define F8		(0x18 + FLAG_EXT)	/* F8		*/
#define F9		(0x19 + FLAG_EXT)	/* F9		*/
#define F10		(0x1A + FLAG_EXT)	/* F10		*/
#define F11		(0x1B + FLAG_EXT)	/* F11		*/
#define F12		(0x1C + FLAG_EXT)	/* F12		*/

/* Control Pad */
#define PRINTSCREEN	(0x1D + FLAG_EXT)	/* Print Screen	*/
#define PAUSEBREAK	(0x1E + FLAG_EXT)	/* Pause/Break	*/
#define INSERT		(0x1F + FLAG_EXT)	/* Insert	*/
#define DELETE		(0x20 + FLAG_EXT)	/* Delete	*/
#define HOME		(0x21 + FLAG_EXT)	/* Home		*/
#define END		(0x22 + FLAG_EXT)	/* End		*/
#define PAGEUP		(0x23 + FLAG_EXT)	/* Page Up	*/
#define PAGEDOWN	(0x24 + FLAG_EXT)	/* Page Down	*/
#define UP		(0x25 + FLAG_EXT)	/* Up		*/
#define DOWN		(0x26 + FLAG_EXT)	/* Down		*/
#define LEFT		(0x27 + FLAG_EXT)	/* Left		*/
#define RIGHT		(0x28 + FLAG_EXT)	/* Right	*/

/* ACPI keys */
#define POWER		(0x29 + FLAG_EXT)	/* Power	*/
#define SLEEP		(0x2A + FLAG_EXT)	/* Sleep	*/
#define WAKE		(0x2B + FLAG_EXT)	/* Wake Up	*/

/* Num Pad */
#define PAD_SLASH	(0x2C + FLAG_EXT)	/* /		*/
#define PAD_STAR	(0x2D + FLAG_EXT)	/* *		*/
#define PAD_MINUS	(0x2E + FLAG_EXT)	/* -		*/
#define PAD_PLUS	(0x2F + FLAG_EXT)	/* +		*/
#define PAD_ENTER	(0x30 + FLAG_EXT)	/* Enter	*/
#define PAD_DOT		(0x31 + FLAG_EXT)	/* .		*/
#define PAD_0		(0x32 + FLAG_EXT)	/* 0		*/
#define PAD_1		(0x33 + FLAG_EXT)	/* 1		*/
#define PAD_2		(0x34 + FLAG_EXT)	/* 2		*/
#define PAD_3		(0x35 + FLAG_EXT)	/* 3		*/
#define PAD_4		(0x36 + FLAG_EXT)	/* 4		*/
#define PAD_5		(0x37 + FLAG_EXT)	/* 5		*/
#define PAD_6		(0x38 + FLAG_EXT)	/* 6		*/
#define PAD_7		(0x39 + FLAG_EXT)	/* 7		*/
#define PAD_8		(0x3A + FLAG_EXT)	/* 8		*/
#define PAD_9		(0x3B + FLAG_EXT)	/* 9		*/
#define PAD_UP		PAD_8			/* Up		*/
#define PAD_DOWN	PAD_2			/* Down		*/
#define PAD_LEFT	PAD_4			/* Left		*/
#define PAD_RIGHT	PAD_6			/* Right	*/
#define PAD_HOME	PAD_7			/* Home		*/
#define PAD_END		PAD_1			/* End		*/
#define PAD_PAGEUP	PAD_9			/* Page Up	*/
#define PAD_PAGEDOWN	PAD_3			/* Page Down	*/
#define PAD_INS		PAD_0			/* Ins		*/
#define PAD_MID		PAD_5			/* Middle key	*/
#define PAD_DEL		PAD_DOT			/* Del		*/

/* Keymap for US MF-2 keyboard. */

const static u32 keymap[NR_SCAN_CODES] = {

/* scan-code                    !Shift          */ 
/* =============================================*/
/* 0x00 - none          */      0,              
/* 0x01 - ESC           */      ESC,            
/* 0x02 - '1'           */      '1',            
/* 0x03 - '2'           */      '2',            
/* 0x04 - '3'           */      '3',            
/* 0x05 - '4'           */      '4',            
/* 0x06 - '5'           */      '5',            
/* 0x07 - '6'           */      '6',            
/* 0x08 - '7'           */      '7',            
/* 0x09 - '8'           */      '8',            
/* 0x0A - '9'           */      '9',            
/* 0x0B - '0'           */      '0',            
/* 0x0C - '-'           */      '-',            
/* 0x0D - '='           */      '=',            
/* 0x0E - BS            */      BACKSPACE,      
/* 0x0F - TAB           */      TAB,            
/* 0x10 - 'q'           */      'q',            
/* 0x11 - 'w'           */      'w',            
/* 0x12 - 'e'           */      'e',            
/* 0x13 - 'r'           */      'r',            
/* 0x14 - 't'           */      't',            
/* 0x15 - 'y'           */      'y',            
/* 0x16 - 'u'           */      'u',            
/* 0x17 - 'i'           */      'i',            
/* 0x18 - 'o'           */      'o',            
/* 0x19 - 'p'           */      'p',            
/* 0x1A - '['           */      '[',            
/* 0x1B - ']'           */      ']',            
/* 0x1C - CR/LF         */      ENTER,          
/* 0x1D - l. Ctrl       */      CTRL_L,         
/* 0x1E - 'a'           */      'a',            
/* 0x1F - 's'           */      's',            
/* 0x20 - 'd'           */      'd',            
/* 0x21 - 'f'           */      'f',            
/* 0x22 - 'g'           */      'g',            
/* 0x23 - 'h'           */      'h',            
/* 0x24 - 'j'           */      'j',            
/* 0x25 - 'k'           */      'k',            
/* 0x26 - 'l'           */      'l',            
/* 0x27 - ';'           */      ';',            
/* 0x28 - '\''          */      '\'',           
/* 0x29 - '`'           */      '`',            
/* 0x2A - l. SHIFT      */      SHIFT_L,        
/* 0x2B - '\'           */      '\\',           
/* 0x2C - 'z'           */      'z',            
/* 0x2D - 'x'           */      'x',            
/* 0x2E - 'c'           */      'c',            
/* 0x2F - 'v'           */      'v',            
/* 0x30 - 'b'           */      'b',            
/* 0x31 - 'n'           */      'n',            
/* 0x32 - 'm'           */      'm',            
/* 0x33 - ','           */      ',',            
/* 0x34 - '.'           */      '.',            
/* 0x35 - '/'           */      '/',            
/* 0x36 - r. SHIFT      */      SHIFT_R,        
/* 0x37 - '*'           */      '*',            
/* 0x38 - ALT           */      ALT_L,          
/* 0x39 - ' '           */      ' ',            
/* 0x3A - CapsLock      */      CAPS_LOCK,      
/* 0x3B - F1            */      F1,             
/* 0x3C - F2            */      F2,             
/* 0x3D - F3            */      F3,             
/* 0x3E - F4            */      F4,             
/* 0x3F - F5            */      F5,             
/* 0x40 - F6            */      F6,             
/* 0x41 - F7            */      F7,             
/* 0x42 - F8            */      F8,             
/* 0x43 - F9            */      F9,             
/* 0x44 - F10           */      F10,            
/* 0x45 - NumLock       */      NUM_LOCK,       
/* 0x46 - ScrLock       */      SCROLL_LOCK,    
/* 0x47 - Home          */      PAD_HOME,       
/* 0x48 - CurUp         */      PAD_UP,         
/* 0x49 - PgUp          */      PAD_PAGEUP,     
/* 0x4A - '-'           */      PAD_MINUS,      
/* 0x4B - Left          */      PAD_LEFT,       
/* 0x4C - MID           */      PAD_MID,        
/* 0x4D - Right         */      PAD_RIGHT,      
/* 0x4E - '+'           */      PAD_PLUS,       
/* 0x4F - End           */      PAD_END,        
/* 0x50 - Down          */      PAD_DOWN,       
/* 0x51 - PgDown        */      PAD_PAGEDOWN,   
/* 0x52 - Insert        */      PAD_INS,        
/* 0x53 - Delete        */      PAD_DOT,        
/* 0x54 - Enter         */      0,              
/* 0x55 - ???           */      0,              
/* 0x56 - ???           */      0,              
/* 0x57 - F11           */      F11,            
/* 0x58 - F12           */      F12,            
/* 0x59 - ???           */      0,              
/* 0x5A - ???           */      0,              
/* 0x5B - ???           */      0,              
/* 0x5C - ???           */      0,              
/* 0x5D - ???           */      0,              
/* 0x5E - ???           */      0,              
/* 0x5F - ???           */      0,              
/* 0x60 - ???           */      0,              
/* 0x61 - ???           */      0,              
/* 0x62 - ???           */      0,              
/* 0x63 - ???           */      0,              
/* 0x64 - ???           */      0,              
/* 0x65 - ???           */      0,              
/* 0x66 - ???           */      0,              
/* 0x67 - ???           */      0,              
/* 0x68 - ???           */      0,              
/* 0x69 - ???           */      0,              
/* 0x6A - ???           */      0,              
/* 0x6B - ???           */      0,              
/* 0x6C - ???           */      0,              
/* 0x6D - ???           */      0,              
/* 0x6E - ???           */      0,              
/* 0x6F - ???           */      0,              
/* 0x70 - ???           */      0,              
/* 0x71 - ???           */      0,              
/* 0x72 - ???           */      0,              
/* 0x73 - ???           */      0,              
/* 0x74 - ???           */      0,              
/* 0x75 - ???           */      0,              
/* 0x76 - ???           */      0,              
/* 0x77 - ???           */      0,              
/* 0x78 - ???           */      0,              
/* 0x78 - ???           */      0,              
/* 0x7A - ???           */      0,              
/* 0x7B - ???           */      0,              
/* 0x7C - ???           */      0,              
/* 0x7D - ???           */      0,              
/* 0x7E - ???           */      0,              
/* 0x7F - ???           */      0,              
};
/*====================================================================
                        Appendix: Scan code set 1
 *====================================================================
KEY     MAKE    BREAK|  KEY     MAKE    BREAK | KEY     MAKE    BREAK
---------------------|------------------------|-----------------------
A       1E      9E   |  9       0A      8A    | [       1A      9A
B       30      B0   |  `       29      89    | INSERT  E0,52   E0,D2
C       2E      AE   |  -       0C      8C    | HOME    E0,47   E0,C7
D       20      A0   |  =       0D      8D    | PG UP   E0,49   E0,C9
E       12      92   |  \       2B      AB    | DELETE  E0,53   E0,D3
F       21      A1   |  BKSP    0E      8E    | END     E0,4F   E0,CF
G       22      A2   |  SPACE   39      B9    | PG DN   E0,51   E0,D1
H       23      A3   |  TAB     0F      8F    | U ARROW E0,48   E0,C8
I       17      97   |  CAPS    3A      BA    | L ARROW E0,4B   E0,CB
J       24      A4   |  L SHFT  2A      AA    | D ARROW E0,50   E0,D0
K       25      A5   |  L CTRL  1D      9D    | R ARROW E0,4D   E0,CD
L       26      A6   |  L GUI   E0,5B   E0,DB | NUM     45      C5
M       32      B2   |  L ALT   38      B8    | KP /    E0,35   E0,B5
N       31      B1   |  R SHFT  36      B6    | KP *    37      B7
O       18      98   |  R CTRL  E0,1D   E0,9D | KP -    4A      CA
P       19      99   |  R GUI   E0,5C   E0,DC | KP +    4E      CE
Q       10      19   |  R ALT   E0,38   E0,B8 | KP EN   E0,1C   E0,9C
R       13      93   |  APPS    E0,5D   E0,DD | KP .    53      D3
S       1F      9F   |  ENTER   1C      9C    | KP 0    52      D2
T       14      94   |  ESC     01      81    | KP 1    4F      CF
U       16      96   |  F1      3B      BB    | KP 2    50      D0
V       2F      AF   |  F2      3C      BC    | KP 3    51      D1
W       11      91   |  F3      3D      BD    | KP 4    4B      CB
X       2D      AD   |  F4      3E      BE    | KP 5    4C      CC
Y       15      95   |  F5      3F      BF    | KP 6    4D      CD
Z       2C      AC   |  F6      40      C0    | KP 7    47      C7
0       0B      8B   |  F7      41      C1    | KP 8    48      C8
1       02      82   |  F8      42      C2    | KP 9    49      C9
2       03      83   |  F9      43      C3    | ]       1B      9B
3       04      84   |  F10     44      C4    | ;       27      A7
4       05      85   |  F11     57      D7    | '       28      A8
5       06      86   |  F12     58      D8    | ,       33      B3
                     |                        |
6       07      87   |  PRTSCRN E0,2A   E0,B7 | .       34      B4
                     |          E0,37   E0,AA |
                     |                        |
7       08      88   |  SCROLL  46      C6    | /       35      B5
                     |                        |
8       09      89   |  PAUSE   E1,1D         |
                     |          45,E1,  -NONE-|                 
                     |          9D,C5         |
----------------------------------------------------------------------
-----------------
ACPI Scan Codes:
-------------------------------------------
Key             Make Code       Break Code
-------------------------------------------
Power           E0, 5E          E0, DE
Sleep           E0, 5F          E0, DF
Wake            E0, 63          E0, E3
-------------------------------
Windows Multimedia Scan Codes:
-------------------------------------------
Key             Make Code       Break Code
-------------------------------------------
Next Track      E0, 19          E0, 99
Previous Track  E0, 10          E0, 90
Stop            E0, 24          E0, A4
Play/Pause      E0, 22          E0, A2
Mute            E0, 20          E0, A0
Volume Up       E0, 30          E0, B0
Volume Down     E0, 2E          E0, AE
Media Select    E0, 6D          E0, ED
E-Mail          E0, 6C          E0, EC
Calculator      E0, 21          E0, A1
My Computer     E0, 6B          E0, EB
WWW Search      E0, 65          E0, E5
WWW Home        E0, 32          E0, B2
WWW Back        E0, 6A          E0, EA
WWW Forward     E0, 69          E0, E9
WWW Stop        E0, 68          E0, E8
WWW Refresh     E0, 67          E0, E7
WWW Favorites   E0, 66          E0, E6
*=====================================================================================*/

#endif /* MINIOS_KERN_KEYMAP_H */
