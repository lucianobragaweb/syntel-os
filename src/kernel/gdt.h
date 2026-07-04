#ifndef GDT_H
#define GDT_H

#include "types.h"

/* Seletores de segmento (índice na GDT << 3, com RPL nos 2 bits baixos) */
#define SEL_KCODE 0x08  /* ring 0 código */
#define SEL_KDATA 0x10  /* ring 0 dados  */
#define SEL_UCODE 0x1B  /* ring 3 código (0x18 | 3) */
#define SEL_UDATA 0x23  /* ring 3 dados  (0x20 | 3) */
#define SEL_TSS   0x28  /* Task State Segment */

/* Reconstrói a GDT (kernel + user + TSS) e carrega o TSS.
   O bootloader montou uma GDT mínima só com ring 0; aqui o kernel
   instala a definitiva, com os segmentos de usuário e o TSS que a
   CPU consulta para achar a stack de ring 0 numa interrupção. */
void gdt_init(void);

/* Atualiza o esp0 do TSS (stack de kernel usada quando um syscall/IRQ
   vem do ring 3) */
void tss_set_esp0(uint32_t esp0);

#endif
