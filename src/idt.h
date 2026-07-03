#ifndef IDT_H
#define IDT_H

#include "types.h"

typedef struct {
    uint16_t offset_low;  // bits 0-15 do endereço do handler
    uint16_t selector;    // segmento de código do kernel
    uint8_t  zero;        // sempre 0
    uint8_t  type_attr;   // tipo e atributos do gate
    uint16_t offset_high; // bits 16-31 do endereço do handler
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;       // tamanho da IDT - 1
    uint32_t base;        // endereço da IDT
} __attribute__((packed)) idt_ptr_t;

void idt_init();

#endif
