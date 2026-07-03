# PIC — Programmable Interrupt Controller

## O problema

Com a IDT configurada, o processador sabe tratar exceções. Mas exceções são eventos síncronos — acontecem como consequência direta de uma instrução. Para eventos assíncronos — uma tecla pressionada, o timer disparando, o disco terminando uma leitura — precisamos de **interrupções de hardware**.

O problema é que o BIOS configura o PIC com um mapeamento que conflita com nossas exceções:

```
BIOS (padrão):
  IRQ0 (timer)    → vetor 0x08  ← conflita com Double Fault!
  IRQ1 (teclado)  → vetor 0x09  ← conflita com Invalid TSS!
  ...
  IRQ7            → vetor 0x0F
```

Antes de habilitar interrupções com `sti`, precisamos remapear o PIC.

---

## O chip 8259A

O PC tem dois PICs conectados em cascata:

```
CPU ← Master PIC (IRQ0–IRQ7) ← Slave PIC (IRQ8–IRQ15)
                  ↑
            IRQ2 = linha de cascata
```

| IRQ | Dispositivo |
|---|---|
| 0 | Timer (PIT) |
| 1 | Teclado (PS/2) |
| 2 | Cascata (slave) |
| 3 | COM2 |
| 4 | COM1 |
| 6 | Floppy |
| 8 | RTC |
| 12 | Mouse PS/2 |
| 14 | ATA primário |
| 15 | ATA secundário |

### Portas de I/O

```
Master: comando 0x20, dados 0x21
Slave:  comando 0xA0, dados 0xA1
```

---

## Remapeamento

Remapeamos para os vetores imediatamente após as exceções da CPU:

```
Após remapeamento:
  IRQ0 (timer)    → vetor 32 (0x20)
  IRQ1 (teclado)  → vetor 33 (0x21)
  ...
  IRQ15 (ATA)     → vetor 47 (0x2F)
```

A sequência de inicialização usa quatro **ICWs** (Initialization Command Words):

```c
// ICW1: inicializa, modo cascata, aguarda ICW4
outb(PIC1_CMD, 0x11);

// ICW2: offset dos vetores
outb(PIC1_DATA, 0x20);  // master: IRQ0 → vetor 32
outb(PIC2_DATA, 0x28);  // slave:  IRQ8 → vetor 40

// ICW3: linha de cascata
outb(PIC1_DATA, 0x04);  // master: slave no IRQ2 (bit 2)
outb(PIC2_DATA, 0x02);  // slave: identidade de cascata

// ICW4: modo 8086
outb(PIC1_DATA, 0x01);
outb(PIC2_DATA, 0x01);
```

### Por que `io_wait()`?

O PIC 8259A é um chip dos anos 70. Em hardware antigo, ele não consegue processar dois comandos seguidos sem uma pausa mínima. `io_wait()` escreve na porta 0x80 (diagnóstico do POST) — uma operação inofensiva que consome ~1μs e dá tempo ao chip.

---

## EOI — End of Interrupt

Após tratar um IRQ, o PIC precisa ser informado que o handler terminou — caso contrário ele bloqueia todos os IRQs de mesma prioridade ou inferior.

```c
void pic_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_CMD, 0x20);  // avisa o slave primeiro
    outb(PIC1_CMD, 0x20);      // depois o master
}
```

**Esquecer o EOI é o erro mais comum ao trabalhar com o PIC.** O sintoma é o sistema recebendo o primeiro IRQ normalmente e nunca mais recebendo o seguinte.

---

## Registrando um handler de IRQ

O sistema usa uma tabela de ponteiros de função — uma por IRQ:

```c
typedef void (*irq_handler_t)(registers_t *);

static irq_handler_t handlers[16];

void irq_install(int irq, irq_handler_t handler) {
    handlers[irq] = handler;
}
```

Ao chegar um IRQ, o dispatcher em C:
1. Calcula o número do IRQ: `int_no - 32`
2. Chama o handler registrado (se houver)
3. Envia EOI

---

## Timer — IRQ0

O timer (PIT, Programmable Interval Timer) dispara a ~18.2 Hz por padrão. Cada disparo é uma oportunidade para o SO fazer algo útil — atualizar contadores, verificar processos, redesenhar a tela.

Para provar que o pipeline funciona, o handler atualiza um spinner no canto superior direito:

```c
static void timer_handler(registers_t *regs) {
    ticks++;
    static const char spinner[] = "|/-\\";
    volatile char *vga = (volatile char*) 0xb8000;
    vga[79 * 2]     = spinner[ticks % 4];
    vga[79 * 2 + 1] = 0x0A;  // verde brilhante
}
```

O `0x0A` é o atributo de cor VGA: `0x0A = 0000 1010` — foreground verde brilhante (10), background preto (0).

---

## Habilitando interrupções

Após configurar IDT, PIC e handlers, habilitamos interrupções com `sti`:

```c
idt_init();
pic_init();
irq_install(0, timer_handler);
__asm__ volatile("sti");
```

A ordem importa: `sti` deve ser o último passo. Se habilitarmos interrupções antes de configurar o PIC, um IRQ pode chegar sem handler e causar double fault.

---

## Próximo passo

Com o timer funcionando, o próximo passo é o **driver de teclado** — capturar a interrupção IRQ1, ler o scancode da porta 0x60 e convertê-lo em caractere ASCII.
