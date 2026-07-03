BUILD := build
SRC   := src

all: $(BUILD)
	nasm -f bin   $(SRC)/boot.asm  -o $(BUILD)/boot.bin
	nasm -f elf32 $(SRC)/start.asm -o $(BUILD)/start.o
	nasm -f elf32 $(SRC)/isr.asm   -o $(BUILD)/isr_stubs.o
	nasm -f elf32 $(SRC)/irq.asm   -o $(BUILD)/irq_stubs.o
	gcc -ffreestanding -m32 -c $(SRC)/kernel.c   -o $(BUILD)/kernel.o
	gcc -ffreestanding -m32 -c $(SRC)/fb.c       -o $(BUILD)/fb.o
	gcc -ffreestanding -m32 -c $(SRC)/idt.c      -o $(BUILD)/idt.o
	gcc -ffreestanding -m32 -c $(SRC)/isr.c      -o $(BUILD)/isr.o
	gcc -ffreestanding -m32 -c $(SRC)/pic.c      -o $(BUILD)/pic.o
	gcc -ffreestanding -m32 -c $(SRC)/irq.c      -o $(BUILD)/irq.o
	gcc -ffreestanding -m32 -c $(SRC)/keyboard.c -o $(BUILD)/keyboard.o
	ld -m elf_i386 -T $(SRC)/linker.ld -o $(BUILD)/kernel.elf \
		$(BUILD)/start.o $(BUILD)/kernel.o $(BUILD)/fb.o $(BUILD)/idt.o \
		$(BUILD)/isr.o $(BUILD)/isr_stubs.o \
		$(BUILD)/pic.o $(BUILD)/irq.o $(BUILD)/irq_stubs.o \
		$(BUILD)/keyboard.o
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
