BUILD := build
SRC   := src

all: $(BUILD)
	nasm -f bin $(SRC)/boot.asm -o $(BUILD)/boot.bin
	gcc -ffreestanding -m32 -c $(SRC)/kernel.c -o $(BUILD)/kernel.o
	gcc -ffreestanding -m32 -c $(SRC)/screen.c -o $(BUILD)/screen.o
	ld -m elf_i386 -T $(SRC)/linker.ld -o $(BUILD)/kernel.elf $(BUILD)/kernel.o $(BUILD)/screen.o
	objcopy -O binary $(BUILD)/kernel.elf $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $(BUILD)/os.img
	truncate -s 1M $(BUILD)/os.img

$(BUILD):
	mkdir -p $(BUILD)

run: all
	-pkill -f qemu-system-i386
	qemu-system-i386 -drive format=raw,file=$(BUILD)/os.img -display curses

clean:
	rm -rf $(BUILD)
