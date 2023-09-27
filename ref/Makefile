#
# make的主文件
#

# 文件夹
# OBJ用于存放编译出来的可重定位文件
OBJDIR	:= obj

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

# 保证all是第一个target，这样make的时候会先执行all
# all的依赖会在之后填充
all :

# OS镜像需要的文件
BOOT_BIN = $(OBJDIR)/boot/boot.bin
LOADER_BIN = $(OBJDIR)/boot/loader.bin

# FAT12镜像文件
IMAGE = $(OBJDIR)/a.img

include boot/Makefrag

$(IMAGE): $(BOOT_BIN) $(LOADER_BIN) 
	@dd if=/dev/zero of=$@ bs=512 count=2880
	@mkfs -t vfat $@
	@dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc
	@sudo mount -o loop $@ /mnt
	@sudo cp $(LOADER_BIN) /mnt -v
	@sudo umount /mnt

all: $(IMAGE)

help: ## show help content
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {sub("\\\\n",sprintf("\n%22c"," "), $$2);printf " \033[36m%-20s\033[0m  %s\n", $$1, $$2}' $(MAKEFILE_LIST)

clean: ## delete all files have been compiled
	@rm -rf $(OBJDIR)

run: $(IMAGE) ## run os normally
	@qemu-system-i386		\
	-boot order=c			\
	-drive file=$<,format=raw	\

gdb: $(IMAGE) ## run os in debug mode 
	@qemu-system-i386		\
	-boot order=c			\
	-drive file=a.img,format=raw	\
	-S -s

monitor: $(IMAGE) ## run monitor for os
	@gdb				\
	-ex 'set tdesc filename target.xml' \
	-ex 'target remote localhost:1234'


# 当make失败的时候，make会将目标删除，这样在创建镜像的时候会非常有用
.DELETE_ON_ERROR:
# .PHONY有个用处就是假设当本文件夹下有all这个文件，
# make all不会判断文件已经被生成而是会去执行all对应的目标
.PHONY: all help clean run gdb monitor 
