BUILD   := build
BOOT    := boot
KSRC    := kernel/src

CC      := gcc
AS      := nasm
LD      := ld

CFLAGS  := -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
           -fno-builtin -fno-asynchronous-unwind-tables -fno-stack-clash-protection \
           -mno-sse -mno-sse2 -mno-mmx -mgeneral-regs-only-no-fpcc 2>/dev/null
CFLAGS  := -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
           -fno-builtin -fno-asynchronous-unwind-tables \
           -mno-sse -mno-sse2 -mno-mmx -O2 -Wall -Wextra

SRCS    := $(wildcard $(KSRC)/*.c)
OBJS    := $(patsubst $(KSRC)/%.c, $(BUILD)/%.o, $(SRCS))

KERNEL_LBA := 64
IMG     := $(BUILD)/kikos.img

.PHONY: all run runq clean shot

all: $(IMG)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/stage2.bin: $(BOOT)/stage2.asm | $(BUILD)
	$(AS) -f bin $< -o $@ -D KERN_SECTORS=0 -D KERN_BYTES=512

$(BUILD)/%.o: $(KSRC)/%.c $(wildcard $(KSRC)/*.h) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(BUILD)/entry.o $(BUILD)/isr.o $(OBJS) kernel/linker.ld
	$(LD) -m elf_i386 -T kernel/linker.ld -nostdlib -o $@ $(BUILD)/entry.o $(BUILD)/isr.o $(OBJS)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	objcopy -O binary $< $@
	python3 scripts/padcheck.py $@ 458752

$(IMG): $(BUILD)/kernel.bin $(BUILD)/stage1.bin $(BUILD)/stage2.final.bin
	cat $(BUILD)/stage1.bin $(BUILD)/stage2.final.bin > $(IMG)
	truncate -s $$(( ($(KERNEL_LBA) * 512) )) $(IMG)
	cat $(BUILD)/kernel.bin >> $(IMG)
	truncate -s 16M $(IMG)
	@echo "==> $(IMG) ready"

$(BUILD)/stage2.final.bin: $(BUILD)/kernel.bin $(BOOT)/stage2.asm
	SECT=$$(( ($$(stat -c%s $(BUILD)/kernel.bin) + 511) / 512 )); \
	BYTES=$$(( ($$(stat -c%s $(BUILD)/kernel.bin) + 3) & ~3 )); \
	$(AS) -f bin -w-error=label-redef-late $(BOOT)/stage2.asm -o $@ -D KERN_SECTORS=$$SECT -D KERN_BYTES=$$BYTES

$(BUILD)/entry.o: $(KSRC)/entry.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/isr.o: $(KSRC)/isr.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/stage1.bin: $(BOOT)/stage1.asm $(BUILD)/stage2.final.bin | $(BUILD)
	S2=$$(( ($$(stat -c%s $(BUILD)/stage2.final.bin) + 511) / 512 )); \
	$(AS) -f bin -w-error=label-redef-late $(BOOT)/stage1.asm -o $@ -D STAGE2_SECTORS=$$S2

run: all
	qemu-system-i386 -m 256 -vga std -drive file=$(IMG),format=raw,if=ide

runq: all
	qemu-system-i386 -m 256 -vga std -display none \
	  -drive file=$(IMG),format=raw,if=ide \
	  -serial file:$(BUILD)/serial.log \
	  -qmp unix:/tmp/kikos.qmp,server,nowait &

clean:
	rm -rf $(BUILD)
