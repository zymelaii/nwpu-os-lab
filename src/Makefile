BOOT_SRCS := $(wildcard boot*.asm)
BOOT_IDLIST := $(patsubst boot_%.asm,%,$(BOOT_SRCS))

LOADER_SRCS := $(wildcard loader*.asm)
LOADER_IDLIST := $(patsubst loader_%.asm,%,$(LOADER_SRCS))

VALID_IDLIST := $(filter $(BOOT_IDLIST), $(LOADER_IDLIST))
IMAGES := $(patsubst %,boot_%.img,$(VALID_IDLIST))

TARGET ?= 1

all: $(IMAGES)

boot_%.img: boot_%.bin loader_%.bin aA1.txt
	@echo - Make $@ from $^
	@dd if=/dev/zero of=$@ bs=512 count=2880
	@mkfs -t vfat $@
	@dd if=$(word 1, $^) of=$@ bs=512 count=1 conv=notrunc
	@sudo mkdir -p /mnt/os-lab
	@sudo mount -o loop $@ /mnt/os-lab
	@sudo cp $(word 2, $^) /mnt/os-lab/loader.bin
	@sudo cp $(word 3, $^) /mnt/os-lab/
	@sudo umount /mnt/os-lab
	@sudo rm -rf /mnt/os-lab

%.bin: %.asm
	@echo - Compiling $< to $@
	@nasm -o $@ $<

aA1.txt:
	@echo - Generate aA1.txt
	@cat /dev/urandom | tr -dc '[:alnum:]' | head -c 5120 > $@

clean:
	@echo - Clean outputs
	@rm -f *.bin *.img
	@rm -f aA1.txt

run: boot_$(TARGET).img
	@echo - Run $<
	@qemu-system-i386 -boot order=c -drive file=$<,format=raw

run-debug: boot_$(TARGET).img
	@echo - Run $< '(DEBUG)'
	@qemu-system-i386 -boot order=c -drive file=$<,format=raw -s -S

gdb:
	@echo - Run GDB
	@gdb -ex 'set tdesc filename ../misc/gdb/target.xml' \
		 -ex 'target remote localhost:1234' \
		 -ex 'layout asm' \
		 -ex 'b *0x00090400' \
		 -ex 'c'

.PHONY: all clean run run-debug gdb
