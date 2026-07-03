BUILD := build
SRC   := src

# Headers compartilhados + headers de cada subsistema
CFLAGS := -ffreestanding -m32 \
          -I$(SRC)/include -I$(SRC)/kernel -I$(SRC)/drivers

all: $(BUILD)
	nasm -f bin   $(SRC)/boot/boot.asm   -o $(BUILD)/boot.bin
	nasm -f elf32 $(SRC)/boot/start.asm  -o $(BUILD)/start.o
	nasm -f elf32 $(SRC)/kernel/isr.asm  -o $(BUILD)/isr_stubs.o
	nasm -f elf32 $(SRC)/kernel/irq.asm  -o $(BUILD)/irq_stubs.o
	gcc $(CFLAGS) -c $(SRC)/kernel/kernel.c    -o $(BUILD)/kernel.o
	gcc $(CFLAGS) -c $(SRC)/kernel/idt.c       -o $(BUILD)/idt.o
	gcc $(CFLAGS) -c $(SRC)/kernel/isr.c       -o $(BUILD)/isr.o
	gcc $(CFLAGS) -c $(SRC)/kernel/pic.c       -o $(BUILD)/pic.o
	gcc $(CFLAGS) -c $(SRC)/kernel/irq.c       -o $(BUILD)/irq.o
	gcc $(CFLAGS) -c $(SRC)/drivers/fb.c       -o $(BUILD)/fb.o
	gcc $(CFLAGS) -c $(SRC)/drivers/keyboard.c -o $(BUILD)/keyboard.o
	ld -m elf_i386 -T $(SRC)/boot/linker.ld -o $(BUILD)/kernel.elf \
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
