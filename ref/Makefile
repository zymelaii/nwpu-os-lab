#
# make的主文件
#

# 文件夹
# OBJ用于存放编译出来的可重定位文件
OBJDIR := obj
# INC用于存放各种头文件(*.h)
INCDIR := inc

# 编译以及日常工具
CC	:= gcc
# 汇编器
AS	:= nasm
# 静态库编辑器
AR	:= ar
# 链接器
LD	:= ld
# 复制文件
OBJCOPY	:= objcopy
# 反编译
OBJDUMP	:= objdump
# 查询可重定位文件符号表
NM	:= nm

DEFS	?= 

# gcc的相关命令参数
# $(DEFS) 定义一些可能的参数
# -O0 0优化，保证程序按照代码语义走而不被优化，方便调试
# -fno-builtin 静止使用gcc内置函数，具体查手册
CFLAGS	:= $(CFLAGS) $(DEFS) -O0 -fno-builtin
# -I 编译时去指定文件夹查找头文件
# -MD 一个黑科技暂时可以不需要了解，总之是在头文件依赖变动的时候能够及时更新target
CFLAGS	+= -I $(INCDIR) -MD
# -fno-stack-protector 禁止栈保护（金丝雀保护机制，内核代码扛不住）
CFLAGS	+= -fno-stack-protector
# -std=gnu99 规定编译的语言规范为gnu99
CFLAGS	+= -std=gnu99
# -fno-pie 不创建动态链接库
CFLAGS	+= -fno-pie
# -static 编译静态程序
# -m32 编译32位程序
CFLAGS	+= -static -m32
# -g 打开gdb调试信息，能够允许gdb的时候调试
CFLAGS	+= -g
# 一车的warning，在编译的时候可能会很有用
CFLAGS	+= -Wall -Wno-format -Wno-unused -Werror

# ld链接器的相关命令参数
# -m elf_i386 链接的格式为i386
LDFLAGS	:= -m elf_i386
# -nostdlib 不链接gcc的标准库，用库只能用命令行的
LDFLAGS	+= -nostdlib

# 获取gcc的库文件（除法取模会用到）
GCC_LIB	:= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# 记录每个OBJDIR里存放的每个子文件夹
# 对于这个系统来说，最后的值为./obj/kern和./obj/boot
OBJDIRS	:=

# 保证all是第一个target，这样make的时候会先执行all
# all的依赖会在之后填充
all:
.PHONY: all

# OS镜像需要的文件
BOOT_BIN = $(OBJDIR)/boot/boot.bin
LOADER_BIN = $(OBJDIR)/boot/loader.bin
KERN_BIN = $(OBJDIR)/kern/kernel.bin
KERN_DBG = $(OBJDIR)/kern/kernel.dbg

# OS镜像文件
IMAGE = $(OBJDIR)/a.img

# 导入子目录文件，分别编写方便管理，也让主makefile更加舒服
include boot/Makefrag
include lib/Makefrag
include kern/Makefrag
include user/Makefrag

# 这里all默认依赖IMAGE
all: $(IMAGE)

$(IMAGE): $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(USER_BINS)
	@stat $@ 1>/dev/null 2>/dev/null || dd if=/dev/zero of=$@ bs=512 count=102400
	@mkfs.vfat -F 32 -s8 $@
	@dd if=$(BOOT_BIN) of=$@ bs=1 count=423 seek=90 conv=notrunc
	@sudo mount -o loop $@ /mnt
	@sudo cp $(LOADER_BIN) /mnt -v
	@sudo cp $(KERN_BIN) /mnt -v
	@sudo cp $(USER_BINS) /mnt
	@sudo umount /mnt

help: ## show help content
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {sub("\\\\n",sprintf("\n%22c"," "), $$2);printf " \033[36m%-20s\033[0m  %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: help

clean: ## delete all files have been compiled
	@rm -rf $(OBJDIR)
.PHONY: clean

run: $(IMAGE) ## run os normally
	@qemu-system-i386		\
	-boot order=a			\
	-drive file=$(IMAGE),format=raw	\
	-serial stdio
.PHONY: run

gdb: $(IMAGE) ## run os in debug mode 
	@qemu-system-i386		\
	-boot order=a			\
	-drive file=$(IMAGE),format=raw	\
	-serial stdio			\
	-s -S
.PHONY: gdb

monitor: $(KERN_DBG) ## run monitor for os
	@gdb                            	\
	-ex 'set confirm off'			\
	-ex 'target remote localhost:1234'	\
	-ex 'file $(KERN_DBG)'
.PHONY: monitor

disassemble: $(KERN_DBG) ## disassemble elf file
	@objdump -S $< | less -S			

# 在这些文件生成后Makefile不会主动将其删掉
.PRECIOUS: 	$(OBJDIR)/.vars.%	\
		$(OBJDIR)/kern/%.o $(OBJDIR)/kern/%.d	\
		$(OBJDIR)/user/%.o $(OBJDIR)/user/%.d	\
		$(OBJDIR)/lib/%.o $(OBJDIR)/lib/kern/%.o $(OBJDIR)/lib/user/%.o	\
		$(OBJDIR)/lib/%.d $(OBJDIR)/lib/kern/%.d $(OBJDIR)/lib/user/%.d

# xv6黑科技，获取编译命令，如果命令较新则重新编译所有文件
$(OBJDIR)/.vars.%: FORCE
	@echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PHONY: FORCE

# 当make失败的时候，make会将目标删除，这样在创建镜像的时候会非常有用
.DELETE_ON_ERROR:

# 黑科技时间，获取每个.c对应的头文件依赖
# 挺难整明白的，不建议一开始整明白，反正从xv6上抄的，不明觉厉
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@perl mergedep.pl $@ $^

-include $(OBJDIR)/.deps