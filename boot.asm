[org 0x7c00]
[bits 16]

start:
    mov si, msg
    call print

    ; carrega kernel (setor 2 em diante)
    mov bx, 0x1000   ; endereço onde o kernel será carregado
    mov dh, 2        ; número de setores
    call load_kernel

    jmp 0x0000:0x1000

print:
    mov ah, 0x0e
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

load_kernel:
    mov ah, 0x02
    mov al, dh
    mov ch, 0
    mov dh, 0
    mov cl, 2
    int 0x13
    ret

msg db "Booting kernel...", 0

times 510-($-$$) db 0
dw 0xaa55

