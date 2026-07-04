#ifndef TASK_H
#define TASK_H

#include "types.h"

#define MAX_TASKS 8

/* Estados de uma tarefa */
#define TASK_READY    0  /* pronta para rodar (ou rodando)     */
#define TASK_SLEEPING 1  /* dormindo até um tick futuro         */

/* Registra o fluxo atual (boot/shell) como tarefa 0 */
void task_init(const char *name);

/* Cria uma tarefa nova com stack própria; retorna o id ou -1 */
int task_create(const char *name, void (*entry)(void));

/* Troca para a próxima tarefa (round-robin). Chamado pelo timer. */
void schedule(void);

/* Bloqueia a tarefa atual por ~ms milissegundos, liberando a CPU */
void task_sleep(uint32_t ms);

/* Para o comando ps */
int         task_count(void);
int         task_current(void);
const char *task_name(int i);
const char *task_state_name(int i);

#endif
