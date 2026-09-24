# ==============================================================================
# 0x7C00-64 - Bare-Metal x86_64 Bootloader & C Kernel (GPLv3)
# ==============================================================================

CC      := gcc
LD      := ld
NASM    := nasm
QEMU    := qemu-system-x86_64

# x86_64 freestanding flags
# -mno-red-zone: Prevents interrupts from clobbering the 128-byte stack red zone
# -mno-mmx -mno-sse: Disables SIMD registers before CR4.OSFXSR is configured
# -mcmodel=kernel: Puts code in negative 2GB address space (or use -mcmodel=small)
CFLAGS  := -m64 -ffreestanding -fno-pie -fno-stack-protector \
           -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -Wall -Wextra -O2 -mcmodel=small

LDFLAGS := -m elf_x86_64 -T linker.ld
ASFLAGS := -f elf64

BUILD_DIR := build

TARGET_IMG  := $(BUILD_DIR)/0x7C00-64.img
BOOT_BIN    := $(BUILD_DIR)/boot.bin
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
KERNEL_OBJS := $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o

.PHONY: all run clean debug

all: $(TARGET_IMG)

$(TARGET_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	@mkdir -p $(BUILD_DIR)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	truncate -s 16384 $@
	@echo "[+] Successfully built $(TARGET_IMG)"

$(BOOT_BIN): boot.asm
	@mkdir -p $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

$(BUILD_DIR)/kernel_entry.o: kernel_entry.asm
	@mkdir -p $(BUILD_DIR)
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: kernel.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

run: $(TARGET_IMG)
	$(QEMU) -cpu host -enable-kvm -drive format=raw,file=$(TARGET_IMG)

debug: $(TARGET_IMG)
	$(QEMU) -s -S -drive format=raw,file=$(TARGET_IMG)

clean:
	rm -rf $(BUILD_DIR)
	@echo "[*] Cleaned build artifacts."