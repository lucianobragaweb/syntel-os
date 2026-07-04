#include "syscall.h"
#include "gdt.h"
#include "fb.h"
#include "isr.h"

/* asm em syscall.asm */
extern void syscall_stub(void);
extern void enter_user(uint32_t entry, uint32_t user_stack);
extern void resume_kernel(void);

/* idt.c: registra um gate com DPL escolhido */
extern void idt_set_gate_dpl(uint8_t n, uint32_t handler, uint8_t dpl);

/* stack de kernel usada quando um syscall vem do ring 3 (ss0:esp0) */
static uint8_t kstack[4096];

void syscall_init(void) {
    /* DPL=3: sem isso, `int 0x80` do ring 3 daria General Protection Fault */
    idt_set_gate_dpl(0x80, (uint32_t)syscall_stub, 3);
}

/* Despacha o syscall. `regs` aponta para o pusha do syscall_stub:
   eax = número, ebx = argumento. */
void syscall_dispatch(registers_t *regs) {
    switch (regs->eax) {
    case SYS_PRINT:
        /* ebx é um ponteiro do usuário. Como usamos espaço de
           endereçamento único, o kernel lê direto (num SO real,
           validaria o ponteiro antes). */
        print((const char *)regs->ebx);
        break;
    case SYS_EXIT:
        resume_kernel();  /* não retorna: volta para o exec no shell */
        break;
    }
}

void exec_user(uint32_t entry, uint32_t user_stack) {
    /* topo da stack de kernel para o TSS (cresce para baixo) */
    tss_set_esp0((uint32_t)kstack + sizeof(kstack));
    enter_user(entry, user_stack);
    /* volta aqui quando o programa chama sys_exit */
}
