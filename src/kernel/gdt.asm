[bits 32]

global gdt_flush
; gdt_flush(uint32_t gdt_ptr) — carrega a GDT nova e recarrega TODOS os
; seletores de segmento. O far jump é obrigatório: é ele que faz a CPU
; recarregar CS com o novo seletor de código do kernel.
gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]
    mov ax, 0x10        ; SEL_KDATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.reload    ; SEL_KCODE:offset — recarrega CS
.reload:
    mov ax, 0x28        ; SEL_TSS
    ltr ax              ; carrega o Task Register
    ret
