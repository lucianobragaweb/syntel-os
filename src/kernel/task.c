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
} task_t;

/* implementadas em task.asm */
extern void task_switch(uint32_t *save_esp, uint32_t new_esp);
extern void task_bootstrap(void);

static task_t tasks[MAX_TASKS];
static int current = 0;
static int ntasks  = 0;

void task_init(const char *name) {
    tasks[0].used = 1;
    tasks[0].name = name;
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

    tasks[i].esp  = (uint32_t)sp;
    tasks[i].name = name;
    tasks[i].used = 1;
    ntasks++;

    irq_restore(f);
    return i;
}

/* Round-robin: cada tarefa roda um tick do timer (~55 ms) e passa a vez */
void schedule(void) {
    if (ntasks < 2) return;

    int prev = current;
    int next = current;
    do {
        next = (next + 1) % MAX_TASKS;
    } while (!tasks[next].used);
    if (next == prev) return;

    current = next;
    task_switch(&tasks[prev].esp, tasks[next].esp);
    /* quando ESTA tarefa for escolhida de novo, a execução continua aqui */
}

int         task_count(void)   { return ntasks; }
int         task_current(void) { return current; }
const char *task_name(int i)   { return (i >= 0 && i < MAX_TASKS && tasks[i].used) ? tasks[i].name : 0; }
