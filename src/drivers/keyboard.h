#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);

/* Lê um caractere do buffer do teclado (bloqueia até ter algo) */
char keyboard_getchar(void);

#endif
