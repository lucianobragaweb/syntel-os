#ifndef SYNC_H
#define SYNC_H

#include "types.h"

/* Seção crítica em single-core: desligar interrupções.
   Sem interrupção não há preempção — o trecho vira atômico.

   Uso:
       uint32_t f = irq_save();
       ... trecho crítico ...
       irq_restore(f);

   irq_save devolve o EFLAGS anterior para que irq_restore recoloque
   o flag IF como estava — nunca dê `sti` cego: a função pode ter sido
   chamada com interrupções já desligadas (ex.: dentro de um handler). */

static inline uint32_t irq_save(void) {
    uint32_t flags;
    __asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags) {
    __asm__ volatile("push %0\n\tpopf" : : "r"(flags) : "memory");
}

#endif
