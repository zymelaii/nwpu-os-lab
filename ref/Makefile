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
# -static 编译静态程序
# -m32 编译32位程序
# -fno-pie 不允许动态链接
CFLAGS	+= -static -m32 -fno-pie
# -g 打开gdb调试信息，能够允许gdb的时候调试
CFLAGS	+= -g
# 一车的warning，在编译的时候可能会很有用
CFLAGS	+= -Wall -Wno-format -Wno-unused -Werror

# ld链接器的相关命令参数
# -m elf_i386 链接的格式为i386
LDFLAGS	:= -m elf_i386

# 记录每个OBJDIR里存放的每个子文件夹
# 对于这个系统来说，最后的值为./obj/boot ./obj/kern ./obj/lib
OBJDIRS	:=

# 保证all是第一个target，这样make的时候会先执行all
# all的依赖会在之后填充
all:
.PHONY: all

# OS镜像需要的文件
MBR_BIN = $(OBJDIR)/boot/mbr.bin
BOOT_BIN = $(OBJDIR)/boot/boot.bin
LOADER_BIN = $(OBJDIR)/boot/loader.bin
KERN_BIN = $(OBJDIR)/kern/kernel.bin
KERN_DBG = $(OBJDIR)/kern/kernel.dbg

# OS镜像文件
IMAGE = $(OBJDIR)/a.img
PARTED_IMAGE = test.img

# 导入子目录文件，分别编写方便管理，也让主makefile更加舒服
include boot/Makefrag
include lib/Makefrag
include kern/Makefrag

# 这里all默认依赖IMAGE
all: $(IMAGE)

# 如果你的make命令报错了, 请先执行generate_image.sh生成基础镜像
# Note that the code part in MBR is only 446 bytes in size
#     otherwise partition table would be destroyed
$(IMAGE): $(MBR_BIN) $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(PARTED_IMAGE)
	@if [ ! -f "$(IMAGE)" ]; then cp -f -v $(PARTED_IMAGE) $(IMAGE) ; fi
	@bash ./install_image.sh $(MBR_BIN) $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(IMAGE)

help: ## show help content
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {sub("\\\\n",sprintf("\n%22c"," "), $$2);printf " \033[36m%-20s\033[0m  %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: help

clean: ## delete all files have been compiled
	@rm -rf $(OBJDIR)
.PHONY: clean

run: $(IMAGE) ## run os normally
	@qemu-system-i386		\
	-boot order=a			\
	-drive file=$(IMAGE),format=raw
.PHONY: run

gdb: $(IMAGE) ## run os in debug mode 
	@qemu-system-i386		\
	-boot order=a			\
	-drive file=$(IMAGE),format=raw	\
	-s -S
.PHONY: gdb

# 这个配置文件可以帮助你更方便地调试16位实模式汇编，如果你需要的话，把下面一行加入到gdb命令中
# -ix "gdb_init_real_mode.txt"
monitor: $(KERN_DBG) ## run monitor for os
	@gdb                            	\
	-ex 'set confirm off'			\
	-ex 'target remote localhost:1234'	\
	-ex 'file $(KERN_DBG)'
.PHONY: monitor

disassemble: $(KERN_DBG) ## disassemble elf file
	@objdump -S $< | less -S

test: ## your extend test for kprintf
	@make run DEFS=-DTESTS --no-print-directory

# 在这些文件生成后Makefile不会主动将其删掉
.PRECIOUS: $(OBJDIR)/.vars.% $(OBJDIR)/kernel/% $(OBJDIR)/user/% $(OBJDIR)/lib/%

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