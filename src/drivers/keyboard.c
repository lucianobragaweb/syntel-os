#include "keyboard.h"
#include "irq.h"
#include "io.h"
#include "fb.h"

#define KB_DATA 0x60
#define SC_MAX  58

/* Scancode set 1: índice = scancode, valor = caractere ASCII */
static const char sc_ascii[SC_MAX] = {
/*00*/  0,    0,   '1', '2', '3', '4', '5', '6', '7', '8',
/*0A*/  '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
/*14*/  't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n',  0,
/*1E*/  'a',  's', 'd', 'f', 'g',  'h', 'j', 'k',  'l', ';',
/*28*/  '\'', '`',  0, '\\', 'z', 'x', 'c', 'v',  'b', 'n',
/*32*/  'm',  ',', '.', '/',  0,  '*',  0,  ' '
};

static const char sc_shift[SC_MAX] = {
/*00*/  0,    0,   '!', '@', '#', '$', '%', '^', '&', '*',
/*0A*/  '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
/*14*/  'T',  'Y', 'U', 'I', 'O',  'P', '{', '}', '\n',  0,
/*1E*/  'A',  'S', 'D', 'F', 'G',  'H', 'J', 'K',  'L', ':',
/*28*/  '"',  '~',  0,  '|', 'Z', 'X', 'C', 'V',  'B', 'N',
/*32*/  'M',  '<', '>', '?',  0,  '*',  0,  ' '
};

static int shift = 0;

static void keyboard_handler(registers_t *regs) {
    (void) regs;

    uint8_t sc = inb(KB_DATA);

    /* bit 7 = tecla solta */
    if (sc & 0x80) {
        sc &= 0x7F;
        if (sc == 0x2A || sc == 0x36) shift = 0;
        return;
    }

    /* tecla pressionada */
    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }

    if (sc >= SC_MAX) return;

    char c = shift ? sc_shift[sc] : sc_ascii[sc];
    if (c) putchar(c);
}

void keyboard_init(void) {
    irq_install(1, keyboard_handler);
}
