#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Entrada do mapa E820 (formato definido pelo BIOS) */
typedef struct {
    uint64_t base;   /* endereço físico inicial da região  */
    uint64_t len;    /* tamanho em bytes                   */
    uint32_t type;   /* 1 = RAM utilizável                 */
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

#define E820_COUNT   ((volatile uint32_t*)0x8000)
#define E820_ENTRIES ((e820_entry_t*)0x8004)
#define E820_USABLE  1

/* Lê o mapa E820 e inicializa o heap do kernel (acima de 1 MB) */
void memory_init(void);

/* Total de RAM utilizável em bytes */
uint32_t memory_total(void);

/* Aloca `size` bytes do heap (first-fit com lista de livres) */
void *kmalloc(uint32_t size);

/* Libera um bloco obtido por kmalloc (volta para a lista de livres) */
void kfree(void *ptr);

/* Igual ao kmalloc, mas alinhado a 4 KB (para page tables). Sem free. */
void *kmalloc_a(uint32_t size);

/* Estatísticas do heap para o meminfo */
uint32_t heap_used(void);
uint32_t heap_size(void);

/* Acesso ao mapa cru, para o shell listar as regiões */
uint32_t e820_count(void);
e820_entry_t *e820_entry(uint32_t i);

#endif
