#include "idt.h"

#define IDT_ENTRIES 256
#define KERNEL_CS   0x08  // seletor do segmento de código (segunda entrada da GDT)
#define IDT_GATE    0x8E  // presente (1), ring 0 (00), 32-bit interrupt gate (1110)

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

// handlers definidos em isr.asm
extern void isr0();  extern void isr1();  extern void isr2();
extern void isr3();  extern void isr4();  extern void isr5();
extern void isr6();  extern void isr7();  extern void isr8();
extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14();
extern void isr15(); extern void isr16(); extern void isr17();
extern void isr18(); extern void isr19(); extern void isr20();
extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26();
extern void isr27(); extern void isr28(); extern void isr29();
extern void isr30(); extern void isr31();

// handlers definidos em irq.asm
extern void irq0();  extern void irq1();  extern void irq2();
extern void irq3();  extern void irq4();  extern void irq5();
extern void irq6();  extern void irq7();  extern void irq8();
extern void irq9();  extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14();
extern void irq15();

static void idt_set_gate(uint8_t n, uint32_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = KERNEL_CS;
    idt[n].zero        = 0;
    idt[n].type_attr   = IDT_GATE;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void idt_init() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t) &idt;

    idt_set_gate(0,  (uint32_t) isr0);
    idt_set_gate(1,  (uint32_t) isr1);
    idt_set_gate(2,  (uint32_t) isr2);
    idt_set_gate(3,  (uint32_t) isr3);
    idt_set_gate(4,  (uint32_t) isr4);
    idt_set_gate(5,  (uint32_t) isr5);
    idt_set_gate(6,  (uint32_t) isr6);
    idt_set_gate(7,  (uint32_t) isr7);
    idt_set_gate(8,  (uint32_t) isr8);
    idt_set_gate(9,  (uint32_t) isr9);
    idt_set_gate(10, (uint32_t) isr10);
    idt_set_gate(11, (uint32_t) isr11);
    idt_set_gate(12, (uint32_t) isr12);
    idt_set_gate(13, (uint32_t) isr13);
    idt_set_gate(14, (uint32_t) isr14);
    idt_set_gate(15, (uint32_t) isr15);
    idt_set_gate(16, (uint32_t) isr16);
    idt_set_gate(17, (uint32_t) isr17);
    idt_set_gate(18, (uint32_t) isr18);
    idt_set_gate(19, (uint32_t) isr19);
    idt_set_gate(20, (uint32_t) isr20);
    idt_set_gate(21, (uint32_t) isr21);
    idt_set_gate(22, (uint32_t) isr22);
    idt_set_gate(23, (uint32_t) isr23);
    idt_set_gate(24, (uint32_t) isr24);
    idt_set_gate(25, (uint32_t) isr25);
    idt_set_gate(26, (uint32_t) isr26);
    idt_set_gate(27, (uint32_t) isr27);
    idt_set_gate(28, (uint32_t) isr28);
    idt_set_gate(29, (uint32_t) isr29);
    idt_set_gate(30, (uint32_t) isr30);
    idt_set_gate(31, (uint32_t) isr31);

    // IRQs remapeados para vetores 32-47
    idt_set_gate(32, (uint32_t) irq0);
    idt_set_gate(33, (uint32_t) irq1);
    idt_set_gate(34, (uint32_t) irq2);
    idt_set_gate(35, (uint32_t) irq3);
    idt_set_gate(36, (uint32_t) irq4);
    idt_set_gate(37, (uint32_t) irq5);
    idt_set_gate(38, (uint32_t) irq6);
    idt_set_gate(39, (uint32_t) irq7);
    idt_set_gate(40, (uint32_t) irq8);
    idt_set_gate(41, (uint32_t) irq9);
    idt_set_gate(42, (uint32_t) irq10);
    idt_set_gate(43, (uint32_t) irq11);
    idt_set_gate(44, (uint32_t) irq12);
    idt_set_gate(45, (uint32_t) irq13);
    idt_set_gate(46, (uint32_t) irq14);
    idt_set_gate(47, (uint32_t) irq15);

    __asm__ volatile ("lidt %0" : : "m" (idt_ptr));
}
