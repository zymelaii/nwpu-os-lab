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
TASKNO ?=

CFLAGS := $(CFLAGS) $(DEFS)
CFLAGS += -I$(INCDIR) -MD
CFLAGS += -O0 -fno-builtin -fno-stack-protector
CFLAGS += -std=gnu99
CFLAGS += -nostdinc -nostdlib
CFLAGS += -static -m32 -fno-pie
CFLAGS += -g
CFLAGS += -Werror
CFLAGS += -Wno-unused-variable
CFLAGS += -DTASKNO=$(TASKNO)

ASFLAGS :=

LDFLAGS	:= -m elf_i386
LDFLAGS += -nostdlib

GCC_LIB	:= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

OBJDIRS	:=

BOOT_BIN = $(OBJDIR)/boot/boot.bin
LOADER_BIN = $(OBJDIR)/boot/loader.bin
KERN_BIN = $(OBJDIR)/kern/kernel.bin
KERN_DBG = $(OBJDIR)/kern/kernel.dbg

IMAGE = $(OBJDIR)/a.img

QEMU_FLAGS := -boot order=a
QEMU_FLAGS += -serial file:stdout.log
QEMU_FLAGS += -m 128m
QEMU_FLAGS += -display curses

GDB_FLAGS := -q -nx
GDB_FLAGS += -x  '../misc/gdb/connect-qemu.gdb'
GDB_FLAGS += -x  '../misc/gdb/instr-level.gdb'
#GDB_FLAGS += -ix '../misc/gdb/gdb_init_real_mode.txt'
GDB_FLAGS += -ex 'set pagination off'
GDB_FLAGS += -ex 'set confirm off'
GDB_FLAGS += -ex 'set disassembly-flavor att'
GDB_FLAGS += -ex 'layout asm'
GDB_FLAGS += -ex 'tui reg general'
GDB_FLAGS += -ex 'connect-qemu'
#GDB_FLAGS += -ex 'b *0x7c00'

include boot/Makefrag
include lib/Makefrag
include kern/Makefrag
include user/Makefrag

all: $(IMAGE)
.PHONY: all

clean:
	@rm -rf $(OBJDIR)
.PHONY: clean

$(IMAGE): $(BOOT_BIN) $(LOADER_BIN) $(KERN_BIN) $(USER_BINS)
	@stat $@ 1>/dev/null 2>/dev/null || dd if=/dev/zero of=$@ bs=512 count=102400
	@mkfs.vfat -F 32 -s8 $@
	@dd if=$(BOOT_BIN) of=$@ bs=1 count=423 seek=90 conv=notrunc
	@mkdir -p $(OBJDIR)/mnt
	@sudo mount -o loop $@ $(OBJDIR)/mnt
	@sudo cp $(LOADER_BIN) $(OBJDIR)/mnt -v
	@sudo cp $(KERN_BIN) $(OBJDIR)/mnt -v
	@sudo cp $(USER_BINS) $(OBJDIR)/mnt -v
	@sudo umount $(OBJDIR)/mnt
	@rm -rf $(OBJDIR)/mnt

run: $(IMAGE)
	@qemu-system-i386 $(QEMU_FLAGS) -drive file=$(IMAGE),format=raw
.PHONY: run

run-debug: $(IMAGE)
	@qemu-system-i386 $(QEMU_FLAGS) -drive file=$(IMAGE),format=raw -s -S
.PHONY: run-debug

gdb: $(KERN_DBG)
	@gdb $(GDB_FLAGS) -ex 'file $<'
.PHONY: gdb

disassemble: $(KERN_DBG)
	@objdump -S $< | less -S
.PHONY: disassemble

.PRECIOUS: 	$(OBJDIR)/.vars.%						  \
		$(OBJDIR)/kern/%.o $(OBJDIR)/kern/%.d		  \
		$(OBJDIR)/user/%.o $(OBJDIR)/user/%.d	  	  \
		$(OBJDIR)/lib/%.o $(OBJDIR)/lib/%.d      	  \
		$(OBJDIR)/lib/kern/%.o $(OBJDIR)/lib/kern/%.d \
		$(OBJDIR)/lib/user/%.o $(OBJDIR)/lib/user/%.d

$(OBJDIR)/.vars.%: FORCE
	@echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PHONY: FORCE

.DELETE_ON_ERROR:

$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@perl mergedep.pl $@ $^

-include $(OBJDIR)/.deps
