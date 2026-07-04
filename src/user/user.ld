ENTRY(_start)

SECTIONS
{
  /* Programas de usuário vivem em 0x400000 (4 MB). O kernel carrega o
     binário aqui e marca a região como acessível em ring 3. */
  . = 0x400000;

  .text : {
    *(.text.start)   /* _start primeiro: é o ponto de entrada */
    *(.text*)
  }
  .rodata : { *(.rodata*) }
  .data   : { *(.data*)   }
  .bss    : { *(.bss*)    }
}
