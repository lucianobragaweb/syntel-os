#ifndef ISR_H
#define ISR_H

#include "types.h"

// Layout exato do que está na pilha quando isr_handler é chamado.
// A ordem reflete a sequência de pushes em isr_common (do último para o primeiro).
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;                         // empurrados pela CPU
} registers_t;

void isr_handler(registers_t *regs);

#endif
