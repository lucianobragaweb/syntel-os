[bits 32]

global syscall_stub
global enter_user
global resume_kernel
extern syscall_dispatch

; ---- int 0x80: ponto de entrada dos syscalls ----
; Ring 3 executa `int 0x80`. A CPU troca para a stack de ring 0 (ss0:esp0
; do TSS), empurra ss/esp/eflags/cs/eip e salta para cá. eax = número do
; syscall, ebx = argumento.
syscall_stub:
    pusha
    push ds            ; mesmo frame que irq_common: gs,fs,es,ds acima do pusha
    push es
    push fs
    push gs            ; → casa com o layout de registers_t (isr.h)
    mov cx, 0x10        ; SEL_KDATA — recarrega segmentos de dados do kernel
    mov ds, cx
    mov es, cx
    push esp            ; ponteiro para os registradores salvos
    call syscall_dispatch
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret               ; volta para o ring 3

; ---- enter_user(uint32_t entry, uint32_t user_stack) ----
; Salva o contexto do kernel (para o resume_kernel voltar no sys_exit) e
; salta para ring 3 forjando um frame de iret. eflags SEM o bit IF: o
; programa roda com interrupções desligadas (evita preempção em ring 3,
; que exigiria kernel stack por tarefa).
saved_kernel_esp: dd 0

enter_user:
    push ebp
    push ebx
    push esi
    push edi
    mov [saved_kernel_esp], esp

    mov ecx, [esp+20]  ; entry      (4 pushes + retaddr = 20 bytes)
    mov edx, [esp+24]  ; user_stack

    mov ax, 0x23       ; SEL_UDATA — segmentos de dados do usuário
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23          ; ss  (user data)
    push edx           ; esp (topo da stack do usuário)
    push 0x2           ; eflags: IF=0 (sem interrupções em ring 3)
    push 0x1B          ; cs  (user code)
    push ecx           ; eip (entry point)
    iret               ; salta para ring 3

; ---- resume_kernel() ----
; Chamado no sys_exit. Restaura o esp do kernel salvo em enter_user e
; "retorna" de enter_user, de volta para quem chamou exec (o shell).
resume_kernel:
    mov esp, [saved_kernel_esp]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
