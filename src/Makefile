OBJDIR := obj
INCDIR := inc

CC	:= gcc
AS	:= nasm
AR	:= ar
LD	:= ld
NM	:= nm
OBJCOPY	:= objcopy
OBJDUMP	:= objdump

DEFS ?=

CFLAGS := $(CFLAGS) $(DEFS)
CFLAGS += -I$(INCDIR) -MD
CFLAGS += -O0 -fno-builtin -fno-stack-protector
CFLAGS += -std=gnu99
CFLAGS += -nostdinc -nostdlib
CFLAGS += -static -m32 -fno-pie
CFLAGS += -g
CFLAGS += -Wall -Werror

LDFLAGS	:= -m elf_i386

OBJDIRS	:=

MBR_BIN    := $(OBJDIR)/boot/mbr.bin
BOOT_BIN   := $(OBJDIR)/boot/boot.bin
LOADER_BIN := $(OBJDIR)/boot/loader.bin
KERN_BIN   := $(OBJDIR)/kern/kernel.bin
KERN_DBG   := $(OBJDIR)/kern/kernel.dbg

IMAGE 		 := $(OBJDIR)/a.img
PARTED_IMAGE := test.img
IMAGE_DEPS   := $(MBR_BIN) $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(PARTED_IMAGE)

include boot/Makefrag
include lib/Makefrag
include kern/Makefrag

all: $(IMAGE)
.PHONY: all

$(IMAGE): $(IMAGE_DEPS)
	@if [ ! -f "$(IMAGE)" ]; then cp -f -v $(PARTED_IMAGE) $(IMAGE) ; fi
	@bash ./install_image.sh $(MBR_BIN) $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(IMAGE)

clean:
	@rm -rf $(OBJDIR)
.PHONY: clean

run: $(IMAGE)
	@qemu-system-i386				\
	-boot order=a					\
	-drive file=$(IMAGE),format=raw	\
	-s -S 							\
	-display curses
.PHONY: gdb

gdb: $(KERN_DBG)
	@gdb										\
	-x  '../misc/gdb/connect-qemu.gdb'			\
	-ex 'set pagination off' 					\
	-ex 'set confirm off'						\
	-ex 'layout asm' 							\
	-ex 'connect-qemu'							\
	-ex 'file $(KERN_DBG)'
.PHONY: gdb

disassemble: $(KERN_DBG)
	@objdump -S $< | less -S

test:
	@make run DEFS=-DTESTS --no-print-directory

.PRECIOUS: $(OBJDIR)/.vars.% $(OBJDIR)/kernel/% $(OBJDIR)/user/% $(OBJDIR)/lib/%

$(OBJDIR)/.vars.%: FORCE
	@echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PHONY: FORCE

.DELETE_ON_ERROR:

$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@perl mergedep.pl $@ $^

-include $(OBJDIR)/.deps
