# Teclado — IRQ1 e Scancodes

## O que acontece quando você pressiona uma tecla

O teclado PS/2 é conectado ao controlador de teclado (Intel 8042) que, por sua vez, gera o **IRQ1** toda vez que uma tecla é pressionada ou solta. O fluxo é:

```
Tecla pressionada
  → controlador PS/2 (8042)
    → sinaliza IRQ1 ao PIC
      → PIC entrega vetor 33 à CPU
        → CPU executa irq1 stub
          → keyboard_handler() é chamado
```

---

## Scancodes

O teclado PS/2 não envia caracteres ASCII — ele envia **scancodes**, números que identificam a tecla física. O **Scancode Set 1** (padrão de compatibilidade XT) é o usado por padrão:

```
Tecla pressionada → scancode com bit 7 = 0  (ex: 'a' = 0x1E)
Tecla solta       → scancode com bit 7 = 1  (ex: 'a' solto = 0x9E)
```

O mapa de algumas teclas:

| Scancode | Tecla | | Scancode | Tecla |
|---|---|---|---|---|
| 0x01 | ESC | | 0x1C | Enter |
| 0x02–0x0B | 1–9, 0 | | 0x2A | Shift esq. |
| 0x0E | Backspace | | 0x36 | Shift dir. |
| 0x10–0x19 | q–p | | 0x39 | Espaço |
| 0x1E–0x26 | a–l | | | |

---

## Lendo o scancode

Quando o IRQ1 dispara, lemos o scancode da **porta 0x60** (porta de dados do controlador PS/2):

```c
uint8_t sc = inb(0x60);
```

O handler então:
1. Verifica o bit 7 → se 1, é tecla solta (atualiza estado do Shift, ignora o resto)
2. Verifica se é Shift (0x2A ou 0x36) → atualiza flag
3. Consulta a tabela de conversão `sc_ascii[]` ou `sc_shift[]`
4. Chama `putchar(c)` para exibir o caractere

---

## Tabelas de conversão

Usamos dois arrays estáticos indexados pelo scancode:

```c
static const char sc_ascii[58] = {
    0, 0, '1', '2', '3', ... 'q', 'w', 'e', ... 'a', 's', ...
};

static const char sc_shift[58] = {
    0, 0, '!', '@', '#', ... 'Q', 'W', 'E', ... 'A', 'S', ...
};
```

Scancode 0 no array significa "sem caractere ASCII" (teclas especiais como Ctrl, Alt, F1–F12).

---

## Estado de Shift

Shift é uma **tecla modificadora** — ela não tem caractere próprio, mas muda o significado das outras:

```c
static int shift = 0;

if (sc == 0x2A || sc == 0x36) { shift = 1; return; }  // pressionou
if (sc & 0x80) {
    if ((sc & 0x7F) == 0x2A || (sc & 0x7F) == 0x36) shift = 0;  // soltou
    return;
}

char c = shift ? sc_shift[sc] : sc_ascii[sc];
```

---

## Backspace

O caractere `\b` (0x08) tem tratamento especial em `screen.c`:

```c
if (c == '\b') {
    if (col > 0) col--;
    else if (row > 0) { row--; col = COLS - 1; }
    // apaga o caractere anterior escrevendo um espaço
    video[(row * COLS + col) * 2] = ' ';
    return;
}
```

---

## Próximo passo

Com o teclado funcionando, o próximo passo natural é implementar um **shell mínimo**: ler uma linha inteira de texto, identificar um comando, e executar alguma ação. Isso une teclado, tela e um laço de leitura — os pilares de qualquer interface de texto.
