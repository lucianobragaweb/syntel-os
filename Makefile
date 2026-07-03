all:
	nasm -f bin boot.asm -o boot.bin
	gcc -ffreestanding -m32 -c kernel.c -o kernel.o
	ld -m elf_i386 -T linker.ld -o kernel.bin kernel.o
	cat boot.bin kernel.bin > os.img

run:
	qemu-system-i386 -drive format=raw,file=os.img
