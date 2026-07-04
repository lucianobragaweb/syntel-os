# IDT — Interrupt Descriptor Table

## O problema

Depois que o kernel inicializa, o processador executa instrução por instrução em linha reta. Se algo inesperado acontece — uma divisão por zero, uma instrução inválida, uma tecla pressionada — o processador não sabe o que fazer. Sem uma tabela de interrupções, ele simplesmente trava ou executa lixo de memória.

A **IDT** é a solução: uma tabela com até 256 entradas que mapeia cada tipo de evento para uma função específica de tratamento.

---

## Tipos de eventos

O x86 divide os eventos em três categorias:

| Tipo | Vetores | Descrição |
|---|---|---|
| **Exceções** | 0–31 | Geradas pela CPU (divisão por zero, page fault, etc.) |
| **Interrupções de software** | 32–127 | Chamadas explícitas com `int N` |
| **Interrupções de hardware** | 32–255 | Sinais de dispositivos (teclado, timer, disco) |

Neste capítulo tratamos apenas as **exceções** (vetores 0–31). O PIC e os IRQs de hardware vêm no próximo capítulo.

---

## Estrutura de uma entrada da IDT

Cada entrada ocupa **8 bytes** e tem o seguinte layout:

```
 Bits 63-48   Bits 47-40    Bits 39-32    Bits 31-16    Bits 15-0
┌────────────┬────────────┬────────────┬────────────┬────────────┐
│ offset_high│ type_attr  │    zero    │  selector  │ offset_low │
│ (bits16-31)│            │  (sempre 0)│  (seg. CS) │ (bits 0-15)│
└────────────┴────────────┴────────────┴────────────┴────────────┘
```

O endereço do handler é partido em `offset_low` e `offset_high` por limitação do design original do 286 — quando a IDT foi criada, os processadores eram 16-bit e o campo de 32-bit foi encaixado depois sem quebrar compatibilidade.

### Campo `type_attr` = `0x8E`

```
Bit 7:   Present = 1      (entrada válida)
Bits 6-5: DPL = 00        (ring 0, somente kernel)
Bit 4:   Storage = 0
Bits 3-0: Gate Type = 1110 (32-bit Interrupt Gate)
```

Um **Interrupt Gate** desabilita interrupções automaticamente ao entrar no handler (`IF=0`) e as restaura no `iret`. Isso evita que um handler seja interrompido por outro evento antes de terminar.

---

## ISR — Interrupt Service Routine

Quando uma exceção dispara, o processador salva automaticamente na pilha:

```
┌──────────┐  ← ESP antes da exceção
│  EFLAGS  │
│    CS    │
│   EIP    │
│ err_code │  ← apenas em algumas exceções
└──────────┘  ← ESP depois da exceção (onde o ISR começa)
```

O problema: os outros registradores (EAX, EBX, etc.) **não são salvos** pela CPU. Se o handler C modificar qualquer registrador, o código interrompido vai encontrar valores errados ao retornar.

A solução é um stub em assembly para cada ISR:

```asm
isr0:
    push dword 0   ; dummy error code (este vetor não tem)
    push dword 0   ; número da exceção
    jmp isr_common
```

E um handler comum que salva tudo antes de chamar o C:

```asm
isr_common:
    pusha           ; salva EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push ds / es / fs / gs

    push esp        ; passa ponteiro para a struct ao handler C
    call isr_handler
    add esp, 4

    pop gs / fs / es / ds
    popa
    add esp, 8      ; remove int_no e err_code
    iret            ; restaura EIP, CS e EFLAGS e retorna
```

### Por que `iret` e não `ret`?

`ret` apenas desempilha o endereço de retorno. `iret` desempilha **EIP + CS + EFLAGS** de uma vez, restaurando o estado completo do processador antes da exceção — incluindo o flag de interrupção `IF`.

---

## Layout da pilha ao entrar em `isr_handler`

A struct `registers_t` mapeia exatamente o que está na pilha nesse momento:

```
Endereço mais alto (empurrado primeiro pela CPU):
  eflags
  cs
  eip
  err_code  (ou 0 se a exceção não gera um)
  int_no
  eax → ecx → edx → ebx → esp → ebp → esi → edi  (pusha)
  ds → es → fs → gs
Endereço mais baixo (topo da pilha, passado como ponteiro)
```

---

## Exceções com e sem error code

Algumas exceções empurram um **error code** automaticamente antes de saltar para o ISR. Outras não. Para manter a pilha com formato uniforme, os stubs sem error code empurram um `0` como placeholder:

| Com error code | Sem error code |
|---|---|
| #8 Double Fault | #0 Division Error |
| #10 Invalid TSS | #6 Invalid Opcode |
| #11 Segment Not Present | #13 já tem |
| #13 General Protection Fault | ... |
| #14 Page Fault | |

---

## Testando

O kernel provoca uma divisão por zero intencional após inicializar a IDT:

```c
int x = 0;
int y = 1 / x;  // dispara exceção #0
```

O resultado esperado na tela:

```
brain-os iniciado
IDT carregada

*** EXCEPTION ***
Division Error (#0x00000000)
Error code: 0x00000000
EIP: 0x00001234
```

---

## Próximo passo

Com as exceções da CPU tratadas, o próximo passo é o **PIC** (Programmable Interrupt Controller) — o chip que gerencia as interrupções de hardware (teclado, timer, disco). Sem remapeá-lo, os IRQs de hardware conflitam com os vetores 0–31 que acabamos de configurar.
