#include "irq.h"
#include "pic.h"

static irq_handler_t handlers[16];

void irq_install(int irq, irq_handler_t handler) {
    handlers[irq] = handler;
}

void irq_handler(registers_t *regs) {
    int irq = regs->int_no - 32;

    if (handlers[irq])
        handlers[irq](regs);

    pic_eoi(irq);
}
