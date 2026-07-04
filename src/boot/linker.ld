ENTRY(_start)

SECTIONS
{
  /* Kernel carregado pelo bootloader em 0x10000 (64 KB).
     Mapa da memória baixa: 0x6000 fonte BIOS, 0x7C00 bootloader,
     0x8000 mapa E820, 0x10000+ kernel, 0x90000 stack (cresce p/ baixo) */
  . = 0x10000;

  .text   : { *(.text*)   }
  .rodata : { *(.rodata*) }
  .data   : { *(.data*)   }
  .bss    : { *(.bss*)    }
}
