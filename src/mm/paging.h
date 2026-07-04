#ifndef PAGING_H
#define PAGING_H

#include "types.h"

/* Monta o identity map (virtual = físico) para toda a RAM utilizável
   e o framebuffer, carrega CR3 e liga o bit de paging no CR0. */
void paging_init(void);

/* Mapeia identity (virtual = físico) o intervalo [start, end) */
void paging_map_range(uint32_t start, uint32_t end);

#endif
