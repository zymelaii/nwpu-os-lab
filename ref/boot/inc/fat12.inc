    ; 下面是 FAT12 磁盘的头
    ; 正常情况下，boot是要对磁盘头的数据进行解析的
    ; 但是出于简单考虑，直接将磁盘头硬编码进来，如果要分析可就太麻烦了，汇编本来就看的头大，还搞那么多未知元
    ; 里面很多信息实际上在boot里用不上，请各位别对里面的参数纠结太多，用到了再查也不迟
    BS_OEMName        DB 'ForrestY'     ; OEM String, 必须 8 个字节
    BPB_BytsPerSec    DW 512            ; 每扇区字节数
    BPB_SecPerClus    DB 1              ; 每簇多少扇区
    BPB_RsvdSecCnt    DW 1              ; Boot 记录占用多少扇区
    BPB_NumFATs       DB 2              ; 共有多少 FAT 表
    BPB_RootEntCnt    DW 224            ; 根目录文件数最大值
    BPB_TotSec16      DW 2880           ; 逻辑扇区总数
    BPB_Media         DB 0xF0           ; 媒体描述符
    BPB_FATSz16       DW 9              ; 每FAT扇区数
    BPB_SecPerTrk     DW 18             ; 每磁道扇区数
    BPB_NumHeads      DW 2              ; 磁头数(面数)
    BPB_HiddSec       DD 0              ; 隐藏扇区数
    BPB_TotSec32      DD 0              ; 如果 wTotalSectorCount 是 0 由这个值记录扇区数
    BS_DrvNum         DB 80h            ; 中断 13 的驱动器号
    BS_Reserved1      DB 0              ; 未使用
    BS_BootSig        DB 29h            ; 扩展引导标记 (29h)
    BS_VolID          DD 0              ; 卷序列号
    BS_VolLab         DB 'OrangeS0.02'  ; 卷标, 必须 11 个字节
    BS_FileSysType    DB 'FAT12   '     ; 文件系统类型, 必须 8个字节  

    ; 文件系统信息存放完毕后后面的内容就可以自由调整了，撒花！

; 原先Orange的代码是按照 主函数->常量->变量->子函数的顺序给出，逻辑很乱
; 这次尝试用C语言风格按照 常量->变量->子函数->主函数的顺序给出，更加符合正常的编程逻辑
;============================================================================
;常量
;================================================================================================

; 这部分请看手册
RootDirSectors            equ 14
SectorNoOfRootDirectory   equ 19
SectorNoOfFAT1            equ 1
DeltaSectorNo             equ 31
;================================================================================================

;============================================================================
;变量
;----------------------------------------------------------------------------
LeftRootDirSectors        dw    RootDirSectors          ; 还未搜索的根目录扇区数
RootDirSectorNow          dw    SectorNoOfRootDirectory ; 目前正在搜索的根目录扇区

;============================================================================
;字符串
;----------------------------------------------------------------------------
KernelFileName            db    "KERNEL  BIN", 0  ; KERNEL.BIN 的文件名(为什么中间有空格请RTFM)
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength             equ    9
BootMessage:              db    "Booting  "    ; 9字节, 不够则用空格补齐. 序号 0
Message1                  db    "Ready.   "    ; 9字节, 不够则用空格补齐. 序号 1
Message2                  db    "No KERNEL"    ; 9字节, 不够则用空格补齐. 序号 2
;============================================================================
; 汇编并不像高级语言一样规范，寄存器忘保存，调用子函数后发现值变了可太痛苦了
; 所以为了减少这份痛苦，这里的所有函数都保证函数除了返回值寄存器其余的主要寄存器都有保护现场
; 保证调用之后不用担心寄存器值变了
;----------------------------------------------------------------------------
; 函数名: Panic
;----------------------------------------------------------------------------
; 作用:
;    当遇到一些程序不可解决恢复的事，就建议直接开摆！输出“No KERNEL”后就地死循环开摆！
Panic:
    mov    dh, 2
    call   DispStr
    jmp    $

;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;    显示一个字符串, 函数开始时 dh 中应该是字符串序号(从0开始)
DispStr:
    pusha
    push   es

    mov    ax, MessageLength
    mul    dh
    add    ax, BootMessage
    mov    bp, ax    
    mov    ax, ds        
    mov    es, ax            ; ES:BP = 串地址
    mov    cx, MessageLength ; CX = 串长度
    mov    ax, 01301h        ; AH = 13,  AL = 01h
    mov    bx, 0007h         ; 页号为0(BH = 0) 黑底白字(BL = 07h)
    mov    dl, 0
    int    10h               ; RTFM

    pop    es
    popa
    ret

;----------------------------------------------------------------------------
; 函数名: DispDot
;----------------------------------------------------------------------------
; 作用:
;    打印一个点
DispDot:
    pusha

    mov    ah, 0Eh       ; `. 每读一个扇区就在 "Booting  " 后面
    mov    al, '.'       ;  | 打一个点, 形成这样的效果:
    mov    bl, 0Fh       ;  | Booting ......
    int    10h           ; /

    popa
    ret
    
;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;    将磁盘的数据读入到内存中
;    ax: 从哪个扇区开始
;    cx: 读入多少个扇区
;    (es:bx): 读入的缓冲区的起始地址
;
;    这里使用的是bios的扩展读功能,写成C语言的结构体是这样的
;    struct buffer_packet {
;        short buffer_packet_size;         /* struct's size（可以为 0x10 或 0x18）*/
;        short sectors;                    /* 读多少个 sectors */
;        char *buffer;                     /* buffer address */
;        long long start_sectors;          /* 从哪个 sector 开始读 */
;        long long *l_buffer;              /* 64 位的 buffer address 这个我们不管！！！*/
;    } buffer;
;    需要注意的是，buffer参数的高4位填充的是段地址(es)，低4位填充的是偏移量(bx)    
;
;    int 13h扩展读功能要求
;    ah = 0x42
;    dl = 驱动号
;    (ds:si)是一个指向buffer的指针
;    中断结束后有一个状态返回，具体请STFW & RTFM
ReadSector:
    pusha

    push   dword 0        
    push   word 0
    push   word ax            ; start_sectors
    push   word es
    push   word bx            ; buffer
    push   word cx            ; sectors           
    push   word 10h           ; buffer_packet_size

    mov    si, sp             ; si <- 指向buffer的地址
    mov    dl, [BS_DrvNum]    ; 驱动号
    mov    ah, 42h            ; 扩展读
    int    13h
    jc     Panic              ; 如果cf位置1，就意味着读入错误，这个时候建议直接开摆
    
    add    sp, 10h

    popa
    ret

;----------------------------------------------------------------------------
; 函数名: GetNextCluster
;----------------------------------------------------------------------------
; 作用:
;    ax存放的是当前的簇(cluster)号，根据当前的簇号在fat表里查找，找到下一个簇的簇号，并将返回值存放在ax
GetNextCluster:
    pusha
    mov    bp, sp

    mov    bx, 3              ; 一个FAT项长度为1.5字节
    mul    bx
    mov    bx, 2              ; ax = floor(clus_number * 1.5)
    div    bx                 ; 这个时候ax里面放着的是FAT项基地址相对于FAT表开头的字节偏移量
                              ; 如果clus_number为奇数，则dx为1，否则为0
    push   dx                 ; 临时保存奇数标识信息
    mov    dx, 0              ; 下面除法要用到
    mov    bx, [BPB_BytsPerSec]
    div    bx                 ; dx:ax / BPB_BytsPerSec
                              ; ax <- 商 (基地址在FAT表的第几个扇区)
                              ; dx <- 余数 (基地址在扇区内的偏移)
    mov    bx, 0              ; bx <- 0 于是, es:bx = BaseOfKernelFile:0
    add    ax, SectorNoOfFAT1 ; 此句之后的 ax 就是FAT项所在的扇区号
    mov    cx, 2              ; 读取FAT项所在的扇区, 一次读两个, 避免在边界
    call   ReadSector         ; 发生错误, 因为一个FAT项可能跨越两个扇区

    mov    bx, dx             ; 将偏移量搬回bx
    mov    ax, [es:bx]
    pop    bx                 ; 取回奇数标识信息
    cmp    bx, 0              ; 如果是第奇数个FAT项还得右移四位，所以这也是为什么FAT12这么辣鸡的原因，
    jz     .EvenCluster       ; 诶……徒增那么多判断，看看隔壁FAT16，FAT32，exFAT，不会发生FAT项跨扇区的情况，也不需要判断第奇偶个
    shr    ax, 4              ; 可能是微软(FAT是微软创建的)第一个亲儿子的原因，有它的历史局限性
.EvenCluster:
    and    ax, 0FFFh          ; 读完需要与一下，因为高位是未定义的
    mov    word [bp + 14], ax ; 这里用了一个技巧，这样在popa的时候ax也顺便更新了

    popa
    ret

;----------------------------------------------------------------------------
; 函数名: StringCmp
;----------------------------------------------------------------------------
; 作用:
;    比较 ds:si 和 es:di 处的字符串（比较长度为11，仅为kernel.bin所用）
;    如果两个字符串相等ax返回1，否则ax返回0
StringCmp:
    pusha
    mov    bp, sp

    mov    cx, 11                 ; 比较长度为11
    cld                           ; 清位保险一下
.STARTCMP:
    lodsb                         ; ds:si -> al
    cmp    al, byte [es:di]
    jnz    .DIFFERENT
    inc    di
    dec    cx
    cmp    cx, 0
    jz     .SAME
    jmp    .STARTCMP
.DIFFERENT:
    mov    word [bp + 14], 0     ; 这里用了一个技巧，这样在popa的时候ax也顺便更新了
    jmp    .ENDCMP
.SAME:
    mov    word [bp + 14], 1     ; 下一步就是ENDCMP了，就懒得jump了
.ENDCMP:
    popa
    ret
;----------------------------------------------------------------------------
; 函数名: LoadKernelFile
;----------------------------------------------------------------------------
; 作用:
;    将kernel的elf文件加载到BaseOfKernelFile:OffsetOfKernelFile
LoadKernelFile: 
    pusha
    push   es

    mov    ax, BaseOfKernelFile
    mov    es, ax                ; es <- BaseOfKernelFile

    ; 清屏
    mov    ax, 0600h             ; AH = 6,  AL = 0h
    mov    bx, 0700h             ; 黑底白字(BL = 07h)
    mov    cx, 0                 ; 左上角: (0, 0)
    mov    dx, 0184fh            ; 右下角: (80, 50)
    int    10h                   ; int 10h

    mov    dh, 0                 ; "Booting  "
    call   DispStr               ; 显示字符串

    mov    ah, 0                 ; ┓
    mov    dl, [BS_DrvNum]       ; ┣ 硬盘复位
    int    13h                   ; ┛
    
; 下面在 A 盘的根目录寻找 KERNEL.BIN
.FindKernelInRootDir:
    mov    ax, [RootDirSectorNow]   ; ax <- 现在正在搜索的扇区号
    mov    bx, OffsetOfKernelFile   ; es:bx = BaseOfKernelFile:OffsetOfKernelFile  
    mov    cx, 1
    call   ReadSector

    mov    si, KernelFileName       ; ds:si -> "KERNEL  BIN"
    mov    di, OffsetOfKernelFile   ; es:di -> BaseOfKernelFile:400h = BaseOfKernelFile*10h+400h
    mov    dx, 10h                  ; 32(目录项大小) * 16(dx) = 512(BPB_BytsPerSec)

.CompareFilename:
    call   StringCmp
    cmp    ax, 1
    jz     .KernelFound             ; ax == 1 -> 比对成了
    dec    dx
    cmp    dx, 0                 
    jz     .GotoNextRootDirSector   ; 该扇区的所有目录项都探索完了，去探索下一个扇区
    add    di, 20h                  ; 32 -> 目录项大小
    jmp    .CompareFilename

.GotoNextRootDirSector:
    inc    word [RootDirSectorNow]      ; 改变正在搜索的扇区号
    dec    word [LeftRootDirSectors]    ; ┓
    cmp    word [LeftRootDirSectors], 0 ; ┣ 判断根目录区是不是已经读完
    jz     Panic                        ; ┛ 如果读完表示没有找到 KERNEL.BIN，就直接开摆
    jmp    .FindKernelInRootDir

.KernelFound:                      ; 找到 KERNEL.BIN 后便来到这里继续
    add    di, 01Ah                ; 0x1a = 28 这个 28 在目录项里偏移量对应的数据是起始簇号（RTFM）
    mov    dx, word [es:di]        ; 起始簇号占2字节，读入到dx里
    mov    bx, OffsetOfKernelFile  ; es:bx = BaseOfKernelFile:OffsetOfKernelFile  

.LoadKernel:
    call   DispDot
    mov    ax, dx                  ; ax <- 数据区簇号
    add    ax, DeltaSectorNo       ; 数据区的簇号需要加上一个偏移量才能得到真正的扇区号
    mov    cx, 1                   ; 一个簇就仅有一个扇区
    call   ReadSector
    mov    ax, dx                  ; ax <- 数据区簇号（在之前ax = 数据区簇号+偏移量）
    call   GetNextCluster          ; 根据数据区簇号获取文件下一个簇的簇号
    mov    dx, ax                  ; dx <- 下一个簇的簇号
    cmp    dx, 0FFFh               ; 判断是否读完了（根据文档理论上dx只要在0xFF8~0xFFF都行，但是这里直接偷懒只判断0xFFF）
    jz     .LoadFinished
    add    bx, [BPB_BytsPerSec]    ; 别忘了更新bx，否则你会发现文件发生复写的情况（指来回更新BaseOfKernelFile:OffsetOfKernelFile ~ BaseOfKernelFile:OffsetOfKernelFile+0x200）
    jmp    .LoadKernel
.LoadFinished:
    mov    dh, 1                   ; "Ready."
    call   DispStr                 ; 显示字符串
    
    pop    es
    popa
    ret