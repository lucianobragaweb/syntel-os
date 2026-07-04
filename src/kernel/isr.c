#include "isr.h"
#include "fb.h"

static const char *exceptions[] = {
    "Division Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
};

void isr_handler(registers_t *regs) {
    print_color("\n*** EXCEPTION ***\n", COLOR_RED);

    if (regs->int_no < 22) {
        print(exceptions[regs->int_no]);
    } else {
        print("Unknown Exception");
    }

    print(" (#");
    print_hex(regs->int_no);
    print(")\n");

    print("Error code: ");
    print_hex(regs->err_code);
    print("\n");

    print("EIP: ");
    print_hex(regs->eip);
    print("\n");

    /* Page fault (#14): CR2 guarda o endereço que causou a falha */
    if (regs->int_no == 14) {
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        print("Endereco que falhou (CR2): ");
        print_hex(cr2);
        print("\n");
        print(regs->err_code & 0x1 ? "Causa: violacao de protecao\n"
                                   : "Causa: pagina nao mapeada\n");
    }

    for (;;);
}
