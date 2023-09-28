BOOT_SRCS := $(wildcard boot*.asm)
BOOT_IDLIST := $(patsubst boot_%.asm,%,$(BOOT_SRCS))

LOADER_SRCS := $(wildcard loader*.asm)
LOADER_IDLIST := $(patsubst loader_%.asm,%,$(LOADER_SRCS))

VALID_IDLIST := $(filter $(BOOT_IDLIST), $(LOADER_IDLIST))
IMAGES := $(patsubst %,boot_%.img,$(VALID_IDLIST))

TARGET ?= 1

all: $(IMAGES)

boot_%.img: boot_%.bin loader_%.bin
	@echo - Make $@ from $^
	@dd if=/dev/zero of=$@ bs=512 count=2880
	@mkfs -t vfat $@
	@dd if=$(word 1, $^) of=$@ bs=512 count=1 conv=notrunc
	@sudo mkdir -p /mnt/os-foobar
	@sudo mount -o loop $@ /mnt/os-foobar
	@sudo cp $(word 2, $^) /mnt/os-foobar/loader.bin
	@sudo umount /mnt/os-foobar
	@sudo rm -rf /mnt/os-foobar

%.bin: %.asm
	@echo - Compiling $< to $@
	@nasm -o $@ $<

clean:
	@echo - Clean outputs
	@rm -f *.bin *.img

run: boot_$(TARGET).img
	@echo - Run $<
	@qemu-system-i386 -boot order=c -drive file=$<,format=raw

.PHONY: all clean run
