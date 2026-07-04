BUILD := build
SRC   := src

# Headers compartilhados + headers de cada subsistema
CFLAGS := -ffreestanding -m32 \
          -I$(SRC)/include -I$(SRC)/kernel -I$(SRC)/drivers -I$(SRC)/mm -I$(SRC)/fs

all: $(BUILD)
	nasm -f bin   $(SRC)/boot/boot.asm   -o $(BUILD)/boot.bin
	nasm -f elf32 $(SRC)/boot/start.asm  -o $(BUILD)/start.o
	nasm -f elf32 $(SRC)/kernel/isr.asm  -o $(BUILD)/isr_stubs.o
	nasm -f elf32 $(SRC)/kernel/irq.asm  -o $(BUILD)/irq_stubs.o
	nasm -f elf32 $(SRC)/kernel/task.asm -o $(BUILD)/task_switch.o
	nasm -f elf32 $(SRC)/kernel/gdt.asm  -o $(BUILD)/gdt_flush.o
	nasm -f elf32 $(SRC)/kernel/syscall.asm -o $(BUILD)/syscall_stub.o
	gcc $(CFLAGS) -c $(SRC)/kernel/kernel.c    -o $(BUILD)/kernel.o
	gcc $(CFLAGS) -c $(SRC)/kernel/shell.c     -o $(BUILD)/shell.o
	gcc $(CFLAGS) -c $(SRC)/kernel/task.c      -o $(BUILD)/task.o
	gcc $(CFLAGS) -c $(SRC)/kernel/gdt.c       -o $(BUILD)/gdt.o
	gcc $(CFLAGS) -c $(SRC)/kernel/syscall.c   -o $(BUILD)/syscall.o
	gcc $(CFLAGS) -c $(SRC)/mm/memory.c        -o $(BUILD)/memory.o
	gcc $(CFLAGS) -c $(SRC)/mm/paging.c        -o $(BUILD)/paging.o
	gcc $(CFLAGS) -c $(SRC)/kernel/idt.c       -o $(BUILD)/idt.o
	gcc $(CFLAGS) -c $(SRC)/kernel/isr.c       -o $(BUILD)/isr.o
	gcc $(CFLAGS) -c $(SRC)/kernel/pic.c       -o $(BUILD)/pic.o
	gcc $(CFLAGS) -c $(SRC)/kernel/irq.c       -o $(BUILD)/irq.o
	gcc $(CFLAGS) -c $(SRC)/drivers/fb.c       -o $(BUILD)/fb.o
	gcc $(CFLAGS) -c $(SRC)/drivers/keyboard.c -o $(BUILD)/keyboard.o
	gcc $(CFLAGS) -c $(SRC)/drivers/ata.c      -o $(BUILD)/ata.o
	gcc $(CFLAGS) -c $(SRC)/fs/syfs.c          -o $(BUILD)/syfs.o
	ld -m elf_i386 -T $(SRC)/boot/linker.ld -o $(BUILD)/kernel.elf \
		$(BUILD)/start.o $(BUILD)/kernel.o $(BUILD)/shell.o \
		$(BUILD)/task.o $(BUILD)/task_switch.o \
		$(BUILD)/gdt.o $(BUILD)/gdt_flush.o \
		$(BUILD)/syscall.o $(BUILD)/syscall_stub.o \
		$(BUILD)/memory.o $(BUILD)/paging.o $(BUILD)/fb.o $(BUILD)/idt.o \
		$(BUILD)/isr.o $(BUILD)/isr_stubs.o \
		$(BUILD)/pic.o $(BUILD)/irq.o $(BUILD)/irq_stubs.o \
		$(BUILD)/keyboard.o $(BUILD)/ata.o $(BUILD)/syfs.o
	objcopy -O binary $(BUILD)/kernel.elf $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $(BUILD)/os.img
	truncate -s 1M $(BUILD)/os.img
	# programa de usuário: compilado e linkado SEPARADO do kernel (base 0x400000)
	gcc $(CFLAGS) -fno-pic -c $(SRC)/user/init.c -o $(BUILD)/init.o
	ld -m elf_i386 -T $(SRC)/user/user.ld -o $(BUILD)/init.elf $(BUILD)/init.o
	objcopy -O binary $(BUILD)/init.elf $(BUILD)/init
	gcc tools/mkfs.c -o $(BUILD)/mkfs
	$(BUILD)/mkfs $(BUILD)/os.img rootfs/* $(BUILD)/init

$(BUILD):
	mkdir -p $(BUILD)

run: all
	-pkill -f qemu-system-i386
	qemu-system-i386 -drive format=raw,file=$(BUILD)/os.img -display curses

clean:
	rm -rf $(BUILD)
