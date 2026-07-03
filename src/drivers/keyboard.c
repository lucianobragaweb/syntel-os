#include "keyboard.h"
#include "irq.h"
#include "io.h"

#define KB_DATA 0x60
#define SC_MAX  58

/* Buffer circular: o handler de interrupção PRODUZ, keyboard_getchar CONSOME.
   O handler nunca escreve na tela — só anota e retorna rápido. */
#define KB_BUF_SIZE 256
static char kb_buf[KB_BUF_SIZE];
static volatile int kb_head = 0;  /* onde o handler escreve  */
static volatile int kb_tail = 0;  /* onde o kernel lê        */

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
    if (!c) return;

    /* deposita no buffer; se cheio, descarta (nunca bloqueia numa interrupção) */
    int next = (kb_head + 1) % KB_BUF_SIZE;
    if (next != kb_tail) {
        kb_buf[kb_head] = c;
        kb_head = next;
    }
}

/* Bloqueia até haver um caractere. O hlt pausa a CPU até a próxima
   interrupção — sem busy-wait queimando ciclos. */
char keyboard_getchar(void) {
    while (kb_head == kb_tail)
        __asm__ volatile("hlt");
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
    return c;
}

void keyboard_init(void) {
    irq_install(1, keyboard_handler);
}
