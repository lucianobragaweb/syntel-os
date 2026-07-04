#include "memory.h"
#include "sync.h"

/* Heap do kernel: região utilizável acima de 1 MB encontrada no E820.
   Alocador "bump": um ponteiro que só avança. Simples e sem free —
   suficiente até implementarmos um alocador de verdade. */

#define HEAP_MIN 0x100000u  /* 1 MB — abaixo disso vive BIOS, kernel, VGA */

static uint32_t total_ram  = 0;
static uint32_t heap_start = 0;
static uint32_t heap_end   = 0;
static uint32_t heap_ptr   = 0;

void memory_init(void) {
    uint32_t n = *E820_COUNT;

    for (uint32_t i = 0; i < n; i++) {
        e820_entry_t *e = &E820_ENTRIES[i];
        if (e->type != E820_USABLE) continue;

        total_ram += (uint32_t)e->len;

        /* heap = maior região utilizável que alcança HEAP_MIN */
        uint32_t base = (uint32_t)e->base;
        uint32_t end  = (uint32_t)(e->base + e->len);
        if (end <= HEAP_MIN) continue;
        if (base < HEAP_MIN) base = HEAP_MIN;
        if (end - base > heap_end - heap_start) {
            heap_start = base;
            heap_end   = end;
        }
    }

    heap_ptr = heap_start;
}

uint32_t memory_total(void) { return total_ram; }

/* Miolo sem lock: ler heap_ptr e avançá-lo precisa ser atômico —
   entre a leitura e a escrita, o timer poderia trocar de tarefa e
   outra chamada devolveria o MESMO bloco (race condition). */
static void *alloc_unlocked(uint32_t size) {
    size = (size + 7) & ~7u;  /* alinha em 8 bytes */
    if (heap_ptr + size > heap_end) return 0;
    void *p = (void *)heap_ptr;
    heap_ptr += size;
    return p;
}

void *kmalloc(uint32_t size) {
    uint32_t f = irq_save();
    void *p = alloc_unlocked(size);
    irq_restore(f);
    return p;
}

/* Alocação alinhada a 4 KB — page tables exigem: a CPU guarda só
   os 20 bits altos do endereço, os 12 baixos são flags.
   O alinhamento e a alocação ficam na MESMA seção crítica. */
void *kmalloc_a(uint32_t size) {
    uint32_t f = irq_save();
    heap_ptr = (heap_ptr + 0xFFF) & ~0xFFFu;
    void *p = alloc_unlocked(size);
    irq_restore(f);
    return p;
}

uint32_t heap_used(void) { return heap_ptr - heap_start; }
uint32_t heap_size(void) { return heap_end - heap_start; }

uint32_t e820_count(void) { return *E820_COUNT; }
e820_entry_t *e820_entry(uint32_t i) { return &E820_ENTRIES[i]; }
