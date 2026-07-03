# syntel-os

Projeto educacional de construção de um sistema operacional x86 do zero — sem frameworks, sem abstrações prontas. Cada linha de código tem um motivo.

---

## O que é um sistema operacional?

Um sistema operacional é o software que fica entre o hardware e os programas do usuário. Ele é responsável por:

- **Inicializar o hardware** — colocar o processador num estado conhecido e previsível
- **Gerenciar recursos** — memória, tempo de CPU, dispositivos de I/O
- **Abstrair o hardware** — oferecer interfaces simples para coisas complexas (ex: "escreva neste arquivo" em vez de "acesse o setor 42 do disco")
- **Isolar processos** — garantir que um programa não interfira em outro

Neste projeto, construímos cada uma dessas responsabilidades do zero, camada por camada.

---

## Arquitetura do projeto

O syntel-os roda em arquitetura **x86 32-bit** (i386), a mesma dos PCs desde os anos 80. É a plataforma mais documentada do mundo para estudo de SOs.

```
┌─────────────────────────────┐
│         Aplicações          │  ← (futuro)
├─────────────────────────────┤
│           Shell             │  ← (futuro)
├─────────────────────────────┤
│   Drivers  │  Filesystem    │  ← (futuro)
├─────────────────────────────┤
│  Memória   │   Processos    │  ← (futuro)
├─────────────────────────────┤
│  Interrupções (IDT/PIC)     │  ← próximo
├─────────────────────────────┤
│  Kernel  │  VGA Driver      │  ← feito ✓
├─────────────────────────────┤
│  Bootloader  │  GDT         │  ← feito ✓
├─────────────────────────────┤
│           Hardware          │
└─────────────────────────────┘
```

---

## Roteiro

### ✅ 1. Bootloader

O BIOS carrega os primeiros 512 bytes do disco para o endereço `0x7c00` e executa. Esse é o bootloader — um programa em assembly 16-bit responsável por:

- Imprimir uma mensagem via BIOS
- Carregar o kernel do disco para a memória (`int 0x13`)
- Configurar a **GDT** (Global Descriptor Table)
- Fazer a transição do processador para **protected mode 32-bit**
- Saltar para o kernel

**Conceitos:** real mode, BIOS interrupts, segmentação de memória, boot signature (`0xaa55`).

---

### ✅ 2. GDT — Global Descriptor Table

Em protected mode, o processador não acessa memória por endereços diretos — ele usa **segmentos** descritos na GDT. Configuramos dois segmentos flat (base 0, limite 4GB): um para código e um para dados.

**Conceitos:** protected mode, segmentação, registradores de segmento (CS, DS, SS).

---

### ✅ 3. VGA Text Mode Driver

A memória de vídeo em modo texto fica no endereço fixo `0xb8000`. Cada caractere ocupa 2 bytes: o ASCII e o atributo de cor. Escrevemos diretamente nessa memória sem passar pelo BIOS.

**Conceitos:** memory-mapped I/O, VGA text mode, atributos de cor.

---

### 🔲 4. IDT — Interrupt Descriptor Table

A IDT é a tabela que diz ao processador o que fazer quando ocorre uma interrupção ou exceção. Sem ela, qualquer evento inesperado (divisão por zero, falha de página, tecla pressionada) trava o sistema sem explicação.

**Conceitos:** exceções de CPU, interrupções de hardware, ISR (Interrupt Service Routine), `lidt`.

---

### 🔲 5. PIC — Programmable Interrupt Controller

O PIC (8259A) é o chip que gerencia as interrupções de hardware (IRQs). Ele recebe sinais do teclado, timer, disco, etc., e os repassa ao processador na ordem correta. Precisamos remapeá-lo para evitar conflito com as exceções da CPU.

**Conceitos:** IRQs, PIC master/slave, remapeamento de interrupções, EOI (End of Interrupt).

---

### 🔲 6. Timer — PIT (Programmable Interval Timer)

O PIT (8253/8254) gera interrupções em intervalos regulares (IRQ0). É a base para medir tempo, implementar delays e futuramente escalonar processos.

**Conceitos:** frequência de clock, divisor, tick do sistema.

---

### 🔲 7. Driver de Teclado

Com IDT e PIC configurados, podemos capturar a interrupção do teclado (IRQ1). O controlador PS/2 envia **scancodes** pela porta `0x60` — precisamos traduzi-los para caracteres ASCII.

**Conceitos:** portas de I/O (`in`/`out`), scancodes, keymap.

---

### 🔲 8. Gerenciamento de Memória

O kernel precisa saber quanta memória há disponível e ser capaz de alocar e liberar blocos. Começamos com um alocador simples (bump allocator) e evoluímos para um heap com `malloc`/`free`.

**Conceitos:** mapa de memória, paginação, heap, fragmentação.

---

### 🔲 9. Shell

Com teclado funcionando e memória gerenciada, podemos construir um interpretador de comandos simples — o ponto de entrada para interagir com o SO.

**Conceitos:** buffer de input, parsing de comandos, loop de REPL.

---

## Ambiente de desenvolvimento

O projeto usa Docker para isolar o ambiente de build e visualização:

```bash
docker compose up
```

Acesse `http://localhost:6080/vnc.html` no browser e clique em **Connect**.

Qualquer alteração em `.c`, `.asm` ou `.ld` reconstrói e reinicia o QEMU automaticamente.

Para rodar localmente (requer nasm, gcc, qemu):

```bash
make run
```
