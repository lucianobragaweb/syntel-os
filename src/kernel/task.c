#include "task.h"
#include "memory.h"
#include "sync.h"

#define STACK_SIZE 8192

/* PCB — Process Control Block. Por enquanto só precisamos do ESP:
   todo o estado da tarefa (registradores, ponto de execução) vive
   na stack dela; o ESP é o fio que puxa tudo de volta. */
typedef struct {
    uint32_t    esp;
    const char *name;
    int         used;
    int         state;      /* TASK_READY / TASK_SLEEPING */
    uint32_t    wake_tick;  /* quando acordar (se dormindo) */
} task_t;

/* implementadas em task.asm */
extern void task_switch(uint32_t *save_esp, uint32_t new_esp);
extern void task_bootstrap(void);

/* o contador do timer, definido em kernel.c */
extern int get_ticks(void);

static task_t tasks[MAX_TASKS];
static int current = 0;
static int ntasks  = 0;

void task_init(const char *name) {
    tasks[0].used  = 1;
    tasks[0].name  = name;
    tasks[0].state = TASK_READY;
    /* tasks[0].esp não é preenchido: será salvo pelo task_switch
       na primeira troca — o fluxo atual JÁ tem uma stack (0x90000) */
    current = 0;
    ntasks  = 1;
}

int task_create(const char *name, void (*entry)(void)) {
    /* seção crítica: o schedule() do timer percorre esta tabela —
       não pode nos ver com a entrada preenchida pela metade */
    uint32_t f = irq_save();

    int i;
    for (i = 0; i < MAX_TASKS && tasks[i].used; i++)
        ;
    if (i == MAX_TASKS) { irq_restore(f); return -1; }

    uint32_t *stack = kmalloc_a(STACK_SIZE);
    if (!stack) { irq_restore(f); return -1; }
    uint32_t *sp = stack + STACK_SIZE / 4;  /* stack cresce para baixo */

    /* Stack forjada: montamos exatamente o que o task_switch espera
       encontrar, como se a tarefa já tivesse sido interrompida antes.
       Na primeira troca: pop edi/esi/ebx/ebp → ret cai no
       task_bootstrap (que liga interrupções) → ret cai na entry. */
    *--sp = (uint32_t)entry;
    *--sp = (uint32_t)task_bootstrap;
    *--sp = 0;  /* ebp */
    *--sp = 0;  /* ebx */
    *--sp = 0;  /* esi */
    *--sp = 0;  /* edi */

    tasks[i].esp   = (uint32_t)sp;
    tasks[i].name  = name;
    tasks[i].used  = 1;
    tasks[i].state = TASK_READY;
    ntasks++;

    irq_restore(f);
    return i;
}

/* Round-robin com estados: acorda quem já passou do wake_tick e
   escolhe a próxima tarefa PRONTA (pula as que dormem). */
void schedule(void) {
    if (ntasks < 2) return;

    uint32_t now = (uint32_t)get_ticks();

    /* acorda dormentes cujo prazo venceu */
    for (int i = 0; i < MAX_TASKS; i++)
        if (tasks[i].used && tasks[i].state == TASK_SLEEPING &&
            (int32_t)(now - tasks[i].wake_tick) >= 0)
            tasks[i].state = TASK_READY;

    /* procura a próxima PRONTA, começando após a atual */
    int prev = current;
    int next = current;
    for (int k = 0; k < MAX_TASKS; k++) {
        next = (next + 1) % MAX_TASKS;
        if (tasks[next].used && tasks[next].state == TASK_READY) break;
    }
    if (next == prev) return;  /* ninguém mais pronto: segue quem está */

    current = next;
    task_switch(&tasks[prev].esp, tasks[next].esp);
    /* quando ESTA tarefa for escolhida de novo, a execução continua aqui */
}

/* Dorme ~ms e cede a CPU. O timer roda a ~18.2 Hz (55 ms/tick). */
void task_sleep(uint32_t ms) {
    uint32_t f = irq_save();
    uint32_t ticks = (ms * 18) / 1000;
    if (ticks == 0) ticks = 1;
    tasks[current].wake_tick = (uint32_t)get_ticks() + ticks;
    tasks[current].state     = TASK_SLEEPING;
    schedule();  /* troca para outra tarefa; volta aqui quando acordar */
    irq_restore(f);
}

int         task_count(void)   { return ntasks; }
int         task_current(void) { return current; }
const char *task_name(int i)   { return (i >= 0 && i < MAX_TASKS && tasks[i].used) ? tasks[i].name : 0; }
const char *task_state_name(int i) {
    if (i < 0 || i >= MAX_TASKS || !tasks[i].used) return 0;
    return tasks[i].state == TASK_SLEEPING ? "dormindo" : "pronta";
}
