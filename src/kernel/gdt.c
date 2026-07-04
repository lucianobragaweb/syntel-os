#include "gdt.h"

/* Descritor de segmento da GDT (8 bytes, formato definido pela CPU) */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

/* Task State Segment: em 32 bits o TSS quase não é usado para troca de
   tarefa (fazemos isso em software), mas a CPU EXIGE um para saber, ao
   entrar em ring 0 vindo do ring 3, qual stack usar: ss0:esp0. */
typedef struct {
    uint32_t prev;
    uint32_t esp0, ss0;   /* stack de ring 0 — o único campo que importa aqui */
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed)) tss_t;

static gdt_entry_t gdt[6];
static gdt_ptr_t   gdt_ptr;
static tss_t       tss;

extern void gdt_flush(uint32_t ptr);  /* gdt.asm: recarrega segmentos + ltr */

static void set_entry(int i, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t gran) {
    gdt[i].limit_low = limit & 0xFFFF;
    gdt[i].base_low  = base & 0xFFFF;
    gdt[i].base_mid  = (base >> 16) & 0xFF;
    gdt[i].access    = access;
    gdt[i].gran      = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].base_high = (base >> 24) & 0xFF;
}

void tss_set_esp0(uint32_t esp0) { tss.esp0 = esp0; }

void gdt_init(void) {
    /* access byte: P|DPL|S|type. 0x9A=code ring0, 0x92=data ring0,
       0xFA=code ring3, 0xF2=data ring3, 0x89=TSS disponível.
       gran 0xCF = granularidade 4KB + 32 bits + limite alto. */
    set_entry(0, 0, 0, 0, 0);                    /* null */
    set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);        /* kernel code */
    set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);        /* kernel data */
    set_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);        /* user code   */
    set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);        /* user data   */

    tss.ss0 = SEL_KDATA;
    tss.esp0 = 0;               /* preenchido por tss_set_esp0 antes do exec */
    tss.iomap_base = sizeof(tss);
    set_entry(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00); /* TSS */

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;
    gdt_flush((uint32_t)&gdt_ptr);
}
