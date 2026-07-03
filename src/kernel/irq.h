#ifndef IRQ_H
#define IRQ_H

#include "isr.h"

typedef void (*irq_handler_t)(registers_t *);

void irq_install(int irq, irq_handler_t handler);
void irq_handler(registers_t *regs);

#endif
