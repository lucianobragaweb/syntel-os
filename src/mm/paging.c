#include "paging.h"
#include "memory.h"
#include "fb.h"

/* Flags das entradas de Page Directory e Page Table */
#define PAGE_PRESENT 0x1  /* página existe (sem isso → page fault)     */
#define PAGE_RW      0x2  /* escrita permitida (sem isso → só leitura) */
#define PAGE_USER    0x4  /* acessível em ring 3 (sem isso → só kernel)*/

/* Page Directory: 1024 entradas, cada uma cobrindo 4 MB.
   Cada entrada aponta para uma Page Table de 1024 páginas de 4 KB. */
static uint32_t *page_dir;

/* Mapeia UMA página: virtual addr → físico addr (identity) */
static void map_page(uint32_t addr) {
    uint32_t pd_i = addr >> 22;          /* 10 bits altos → índice no PD  */
    uint32_t pt_i = (addr >> 12) & 0x3FF; /* 10 bits do meio → índice na PT */
    uint32_t *pt;

    if (page_dir[pd_i] & PAGE_PRESENT) {
        /* PT já existe: extrai o endereço (20 bits altos da entrada) */
        pt = (uint32_t *)(page_dir[pd_i] & ~0xFFFu);
    } else {
        /* primeira página nesta região de 4 MB: cria a PT */
        pt = kmalloc_a(4096);
        for (int i = 0; i < 1024; i++) pt[i] = 0;
        page_dir[pd_i] = (uint32_t)pt | PAGE_PRESENT | PAGE_RW;
    }

    pt[pt_i] = (addr & ~0xFFFu) | PAGE_PRESENT | PAGE_RW;
}

void paging_map_range(uint32_t start, uint32_t end) {
    for (uint32_t a = start & ~0xFFFu; a < end; a += 4096)
        map_page(a);
}

/* Marca [start, end) como acessível em ring 3. Tanto a Page Table
   quanto a entrada do Page Directory precisam da flag USER — a CPU
   checa os DOIS níveis ao traduzir um acesso de ring 3. */
void paging_set_user(uint32_t start, uint32_t end) {
    for (uint32_t a = start & ~0xFFFu; a < end; a += 4096) {
        uint32_t pd_i = a >> 22;
        uint32_t pt_i = (a >> 12) & 0x3FF;
        if (!(page_dir[pd_i] & PAGE_PRESENT)) continue;
        page_dir[pd_i] |= PAGE_USER;
        uint32_t *pt = (uint32_t *)(page_dir[pd_i] & ~0xFFFu);
        pt[pt_i] |= PAGE_USER;
    }
    /* recarrega CR3 para invalidar a TLB (traduções em cache) */
    __asm__ volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax");
}

void paging_init(void) {
    page_dir = kmalloc_a(4096);
    for (int i = 0; i < 1024; i++) page_dir[i] = 0;

    /* Identity map de toda RAM utilizável do E820 — inclui kernel,
       stack, heap (e as próprias page tables que estamos criando) */
    for (uint32_t i = 0; i < e820_count(); i++) {
        e820_entry_t *e = e820_entry(i);
        if (e->type != E820_USABLE) continue;
        paging_map_range((uint32_t)e->base, (uint32_t)(e->base + e->len));
    }

    /* Framebuffer (4 MB a partir do BAR0) — sem isso, primeira
       escrita na tela após ligar o paging seria um page fault */
    uint32_t fb = fb_get_base();
    paging_map_range(fb, fb + 4 * 1024 * 1024);

    /* Liga: CR3 = endereço do PD; CR0 bit 31 = paging enable.
       A partir da próxima instrução, TODO acesso passa pela MMU. */
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
