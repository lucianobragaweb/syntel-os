[org 0x7c00]
[bits 16]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax

    ; Carrega kernel — 30 setores = 15 KB em 0x1000..0x4C00
    mov bx, 0x1000
    mov dh, 30
    call load_kernel

    ; Mapa de memoria E820 → contagem em 0x8000, entradas de 24 bytes em 0x8004
    ; (0x8000 fica acima do bootloader 0x7C00-0x7DFF; nao usar 0x5000 —
    ;  o BSS do kernel cresce ate la e as variaveis seriam sobrescritas!)
    mov di, 0x8004
    xor ebx, ebx        ; continuation: 0 = primeira chamada
    xor bp, bp          ; contador de entradas
.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150 ; assinatura 'SMAP'
    mov ecx, 24
    int 0x15
    jc .e820_done       ; carry = erro ou fim
    inc bp
    add di, 24
    cmp bp, 32          ; limite de seguranca (0x8004 + 32*24 < 0x9000)
    jae .e820_done
    test ebx, ebx       ; ebx = 0 → era a ultima entrada
    jnz .e820_loop
.e820_done:
    mov [0x8000], bp

    ; Copia a fonte 8x16 do BIOS para 0x6000 (256 chars × 16 bytes = 4096 bytes)
    ; ATENÇÃO: destino não pode alcançar 0x7C00, senão sobrescreve este bootloader
    mov ax, 0x1130
    mov bh, 0x06        ; fonte 8x16
    int 0x10            ; ES:BP → fonte
    mov ax, es
    mov ds, ax          ; DS:SI = fonte
    mov si, bp
    xor ax, ax
    mov es, ax          ; ES:DI = 0x0000:0x6000
    mov di, 0x6000
    mov cx, 4096
    rep movsb
    xor ax, ax
    mov ds, ax          ; restaura DS

    ; Bochs VBE via port I/O — funciona em modo real, garantido no QEMU
    ; Porta 0x01CE = índice, 0x01CF = dado
    mov dx, 0x01CE
    mov ax, 4           ; índice VBE_DISPI_INDEX_ENABLE
    out dx, ax
    mov dx, 0x01CF
    xor ax, ax          ; desabilita antes de reconfigurar
    out dx, ax

    mov dx, 0x01CE
    mov ax, 1           ; VBE_DISPI_INDEX_XRES
    out dx, ax
    mov dx, 0x01CF
    mov ax, 1024
    out dx, ax

    mov dx, 0x01CE
    mov ax, 2           ; VBE_DISPI_INDEX_YRES
    out dx, ax
    mov dx, 0x01CF
    mov ax, 768
    out dx, ax

    mov dx, 0x01CE
    mov ax, 3           ; VBE_DISPI_INDEX_BPP
    out dx, ax
    mov dx, 0x01CF
    mov ax, 32          ; 32bpp (mais compatível que 24)
    out dx, ax

    mov dx, 0x01CE
    mov ax, 4           ; VBE_DISPI_INDEX_ENABLE
    out dx, ax
    mov dx, 0x01CF
    mov ax, 0x41        ; VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED
    out dx, ax

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    jmp 0x1000

[bits 16]
load_kernel:
    mov ah, 0x02
    mov al, dh
    mov ch, 0
    mov cl, 2
    mov dh, 0
    int 0x13
    jc disk_error
    ret

disk_error:
    hlt

gdt_start:
    dq 0
gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510-($-$$) db 0
dw 0xaa55
