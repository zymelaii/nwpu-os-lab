;==============================================================================================================================
BaseOfStack			equ	0x07c00		; Boot状态下堆栈基地址
STACK_ADDR			equ	BaseOfStack-28	; 堆栈栈顶

BaseOfBoot			equ	1000h		; added by mingxuan 2020-9-12
OffsetOfBoot			equ	7c00h		; load Boot sector to BaseOfBoot:OffsetOfBoot
OffsetOfActiPartStartSec	equ	7e00h		; 活动分区的起始扇区号相对于BaseOfBoot的偏移量	;added by mingxuan 2020-9-12 
							; 该变量来自分区表，保存在该内存地址，用于在os_boot和loader中查找FAT32文件
BOOT_FAT32_INFO			equ	0x5A		; 位于boot中的FAT32配置信息的长度

OSLOADER_SEG			equ	0x1000		; 段地址
OSLOADER_OFF			equ	0x5000		; 段偏移
OSLOADER_ADDR			equ	0x15000		; loader地址
BUF_OFF				equ	0x0000		; 加载FAT表的临时存储
BUF_ADDR			equ	0x10000		; 加载FAT表的临时存储

DIR_PER_SECTOR			equ	0x10		; 每个扇区所容纳的目录 BYTE

; 扩展磁盘服务所使用的地址包
DAP_SECTOR			equ	8		; 起始扇区号
DAP_BUFFER_SEG			equ	10		; 缓冲区地址
DAP_BUFFER_OFF			equ	12		; 缓冲区地址
DAP_READ_SECTORS		equ	14		; 要处理的扇区数
DAP_PACKET_SIZE			equ	16		; 包的大小为24字节
CURRENT_CLUSTER			equ	20		; 当前正在处理的簇号 DWORD
FAT_START_SECTOR		equ	24		; FAT表的起始扇区号 ;added by mingxuan 2020-9-17
DATA_START_SECTOR		equ	28		; 数据区起始扇区号 ;added by mingxuan 2020-9-17

; 目录项结构
OFF_START_CLUSTER_HIGH		equ	20		; 起始簇号高位  WORD
OFF_START_CLUSTER_LOW		equ	26		; 起始簇号低位  WORD

; 相关常量
DIR_NAME_FREE			equ	0x00		; 该项是空闲的
DIR_ENTRY_SIZE			equ	32		; 每个目录项的尺寸
; 簇属性
CLUSTER_MASK			equ	0FFFFFFFH	; 簇号掩码
CLUSTER_LAST			equ	0FFFFFF8H	;0xFFFFFFF8-0xFFFFFFFF表示文件的最后一个簇

; added by mingxuan 2020-9-12
BPB_BytesPerSec			equ	(OffsetOfBoot + 0xb)	;每扇区字节数
BPB_SecPerClu			equ	(OffsetOfBoot + 0xd)	;每簇扇区数 
BPB_RsvdSecCnt			equ	(OffsetOfBoot + 0xe)	;保留扇区数
BPB_NumFATs			equ	(OffsetOfBoot + 0x10)	;FAT表数
BPB_RootEntCnt			equ	(OffsetOfBoot + 0x11)	;FAT32不使用
BPB_TotSec16			equ	(OffsetOfBoot + 0x13)	;扇区总数
BPB_Media			equ	(OffsetOfBoot + 0x15)	;介质描述符
BPB_FATSz16			equ	(OffsetOfBoot + 0x16)	;每个FAT表的大小扇区数(FAT32不使用)
BPB_SecPerTrk			equ	(OffsetOfBoot + 0x18)	;每磁道扇区数
BPB_NumHeads			equ	(OffsetOfBoot + 0x1a)	;磁头数
BPB_HiddSec			equ	(OffsetOfBoot + 0x1c)	;分区已使用扇区数
BPB_TotSec32			equ	(OffsetOfBoot + 0x20)	;文件系统大小扇区数
BPB_FATSz32			equ	(OffsetOfBoot + 0x24)	;每个FAT表大小扇区数
BPB_ExtFlags			equ	(OffsetOfBoot + 0x28)	;标记
BPB_FSVer			equ	(OffsetOfBoot + 0x2a)	;版本号
BPB_RootClus			equ	(OffsetOfBoot + 0x2c)	;根目录簇号
BPB_FSInfo			equ	(OffsetOfBoot + 0x30)	;FSINFO扇区号
BS_BackBootSec			equ	(OffsetOfBoot + 0x32)	;备份引导扇区位置
BPB_Reserved			equ	(OffsetOfBoot + 0x34)	;未使用
BS_DrvNum			equ	(OffsetOfBoot + 0x40)	;设备号
BS_Reserved1			equ	(OffsetOfBoot + 0x41)	;未使用
BS_BootSig			equ	(OffsetOfBoot + 0x42)	;扩展引导标志
BS_VolID			equ	(OffsetOfBoot + 0x43)	;卷序列号
BS_VolLab			equ	(OffsetOfBoot + 0x47)	;卷标
BS_FilSysType			equ	(OffsetOfBoot + 0x52)	;文件系统类型
;==============================================================================================================================
	org	(07c00h + BOOT_FAT32_INFO)	;FAT322规范规定第90~512个字节(共423个字节)是引导程序
						;modified by mingxuan 2020-9-16
START:	
	cld
	mov	ax, cs
	mov	ds, ax
	mov	ss, ax
	mov	ax, OSLOADER_SEG
	mov	es, ax ;deleted by mingxuan 2020-9-13

	mov	bp, BaseOfStack
	mov	sp, STACK_ADDR

	movzx	eax, word [BPB_RsvdSecCnt]
	mov	[bp - FAT_START_SECTOR], eax

	; 计算数据区起始扇区号 ; added by mingxuan 2020-9-17
	movzx	cx, byte [BPB_NumFATs]
_CALC_DATA_START:
	add	eax, [BPB_FATSz32]
	loop	_CALC_DATA_START

	mov	[bp - DATA_START_SECTOR], eax

	mov	word [bp - DAP_PACKET_SIZE], 10h
	mov	word [bp - DAP_BUFFER_SEG], OSLOADER_SEG

	jmp	_SEARCH_LOADER

LoaderName	db	"LOADER  BIN"	; 第二阶段启动程序 FDOSLDR.BIN
ReadSector:
	pusha
	
	mov	ah, 42h				;ah是功能号，扩展读对应0x42
	lea	si, [bp - DAP_PACKET_SIZE]	;使用扩展int13时DAP结构体首地址传给si
	mov	dl, [BS_DrvNum]			;modified by mingxuan 2020-9-17
	int	13h
	jc	$

	popa
	ret

InlineReadDataSector:
	sub	eax, 2
	movzx	edx, byte [BPB_SecPerClu]
	mul	edx
	add	eax, [bp - DATA_START_SECTOR]
	mov	[bp - DAP_SECTOR], eax

	movzx	edx, byte [BPB_SecPerClu]
	mov	[bp - DAP_READ_SECTORS], dx

	mov	[bp - DAP_BUFFER_OFF], bx
	call	ReadSector
	ret

; 根据簇号计算扇区号
; Comments, added by mingxuan 2020-9-10
;====================================================================
; 检查是否还有下一个簇(读取FAT表的相关信息)
;  N = 数据簇号
;  FAT_BYTES(在FAT表中的偏移)  = N*4  (FAT32)
;  FAT_SECTOR = FAT_BYTES / BPB_BytesPerSec
;  FAT_OFFSET = FAT_BYTES % BPB_BytesPerSec
;====================================================================
NextCluster:					;added by yangxiaofeng 2021-12-1
	pushad
	
	mov	eax, [bp - CURRENT_CLUSTER]
	mov	ebx, 4
	mul	ebx
	movzx	ebx, word [BPB_BytesPerSec]
	div	ebx
	add	eax, [bp - FAT_START_SECTOR]
	mov	[bp - DAP_SECTOR], eax

	mov	word [bp - DAP_READ_SECTORS], 1
	
	mov	word [bp - DAP_BUFFER_OFF], BUF_OFF
	
	call	ReadSector
	mov	bx, dx
	mov	eax, [es:bx + BUF_OFF]
	mov	[bp - DATA_START_SECTOR - 6], eax
	
	popad
	ret 

_SEARCH_LOADER:
	mov	eax, [BPB_RootClus]

_NEXT_ROOT_CLUSTER:			;这个时候eax拿着的是当前的簇号
	mov	[bp - CURRENT_CLUSTER], eax
	mov	bx, BUF_OFF

	call	InlineReadDataSector

	mov	di, BUF_OFF
	; 这里dl拿着的是BPB_SecPerClu
_NEXT_ROOT_SECTOR:
	mov	bl, DIR_PER_SECTOR

_NEXT_ROOT_ENTRY:
	mov	si, LoaderName
	mov	cx, 11
	repe	cmpsb
	and	di, DIR_ENTRY_SIZE
	jcxz	_FOUND_LOADER
	add	di, DIR_ENTRY_SIZE

	dec	bl
	jnz	_NEXT_ROOT_ENTRY

	dec	dl
	jz	_CHECK_NEXT_ROOT_CLUSTER

	jmp	_NEXT_ROOT_SECTOR

_CHECK_NEXT_ROOT_CLUSTER: ; 检查是否还有下一个簇
	call	NextCluster		;added by yangxiaofeng 2021-12-1
	cmp	eax, CLUSTER_LAST	;CX >= 0FFFFFF8H，则意味着没有更多的簇了
	jb	_NEXT_ROOT_CLUSTER
	jmp	$

_FOUND_LOADER:
	movzx	eax, word [es:di + OFF_START_CLUSTER_HIGH]; 起始簇号高32位
	shl	eax, 16
	mov	ax, [es:di + OFF_START_CLUSTER_LOW]	; 起始簇号低32位
	mov	bx, OSLOADER_OFF
;这个时候eax拿着的是当前的簇号，ebx存放加载的地址
_LOAD_LOADER:				
	mov	dword [bp - CURRENT_CLUSTER], eax

	call	InlineReadDataSector

	movzx	eax, byte [BPB_SecPerClu]
	mov	cx, [BPB_BytesPerSec]
	mul	cx
	add	ebx, eax

	call	NextCluster
	cmp	eax, CLUSTER_LAST	;CX >= 0FFFFFF8H，则意味着没有更多的簇了
	jb	_LOAD_LOADER

_RUN_LOADER: 
	jmp	OSLOADER_SEG : OSLOADER_OFF

times	510 - BOOT_FAT32_INFO - ($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw	0xaa55				; 结束标志