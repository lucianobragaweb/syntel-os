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

/* Cada bloco carrega um cabeçalho com seu tamanho. Quando livre, o
   campo `next` encadeia numa lista de livres; quando em uso, é lixo.
   É assim que um alocador sabe, no kfree(ptr), quanto liberar: o
   tamanho está logo antes do ponteiro que devolvemos. */
typedef struct block {
    uint32_t      size;  /* bytes úteis (depois do cabeçalho) */
    struct block *next;  /* próximo livre (só válido se livre) */
} block_t;

#define HDR sizeof(block_t)

static block_t *free_list = 0;

/* Empurra o ponteiro do heap (bump). Base de tudo: só cresce. */
static void *bump(uint32_t size) {
    if (heap_ptr + size > heap_end) return 0;
    void *p = (void *)heap_ptr;
    heap_ptr += size;
    return p;
}

void *kmalloc(uint32_t size) {
    size = (size + 7) & ~7u;  /* alinha em 8 bytes */
    uint32_t f = irq_save();

    /* first-fit: reaproveita o primeiro bloco livre que couber */
    block_t **prev = &free_list;
    for (block_t *b = free_list; b; prev = &b->next, b = b->next) {
        if (b->size >= size) {
            *prev = b->next;           /* desencadeia da lista de livres */
            irq_restore(f);
            return (void *)((uint8_t *)b + HDR);
        }
    }

    /* nenhum bloco livre serve: pega memória nova do heap */
    block_t *b = bump(HDR + size);
    if (!b) { irq_restore(f); return 0; }
    b->size = size;
    irq_restore(f);
    return (void *)((uint8_t *)b + HDR);
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((uint8_t *)ptr - HDR);
    uint32_t f = irq_save();
    b->next   = free_list;   /* devolve para a lista de livres */
    free_list = b;
    irq_restore(f);
}

/* Alocação alinhada a 4 KB — page tables exigem: a CPU guarda só
   os 20 bits altos do endereço, os 12 baixos são flags. Não usa
   cabeçalho: page tables nunca são liberadas. */
void *kmalloc_a(uint32_t size) {
    uint32_t f = irq_save();
    heap_ptr = (heap_ptr + 0xFFF) & ~0xFFFu;
    void *p = bump(size);
    irq_restore(f);
    return p;
}

uint32_t heap_used(void) { return heap_ptr - heap_start; }
uint32_t heap_size(void) { return heap_end - heap_start; }

uint32_t e820_count(void) { return *E820_COUNT; }
e820_entry_t *e820_entry(uint32_t i) { return &E820_ENTRIES[i]; }
