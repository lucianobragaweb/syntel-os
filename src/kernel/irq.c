#include "irq.h"
#include "pic.h"

static irq_handler_t handlers[16];

void irq_install(int irq, irq_handler_t handler) {
    handlers[irq] = handler;
}

void irq_handler(registers_t *regs) {
    int irq = regs->int_no - 32;

    /* EOI ANTES do handler: o timer pode trocar de tarefa dentro do
       handler e só voltar muito depois — o PIC não pode ficar esperando */
    pic_eoi(irq);

    if (handlers[irq])
        handlers[irq](regs);
}
