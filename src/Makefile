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

ASFLAGS :=

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

$(PARTED_IMAGE): generate_image.sh part1.sfdisk
	@bash $<

$(IMAGE): install_image.sh $(IMAGE_DEPS)
	@cp -f -v $(PARTED_IMAGE) $(IMAGE)
	@bash $< $(MBR_BIN) $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(IMAGE)

clean:
	@rm -rf $(OBJDIR) *.img
.PHONY: clean

run: $(IMAGE)
	@qemu-system-i386				\
	-boot order=a					\
	-drive file=$(IMAGE),format=raw	\
	-s -S 							\
	-display curses -m 128m
.PHONY: run

gdb: $(KERN_DBG)
	@gdb -q -nx									\
	-x  '../misc/gdb/connect-qemu.gdb'			\
	-x  '../misc/gdb/instr-level.gdb'			\
	-ix '../misc/gdb/gdb_init_real_mode.txt'	\
	-ex 'set pagination off' 					\
	-ex 'set confirm off'						\
	-ex 'set disassembly-flavor att'			\
	-ex 'layout asm' 							\
	-ex 'tui reg general'						\
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
