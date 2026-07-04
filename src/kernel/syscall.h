#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

/* Números dos syscalls — o contrato entre kernel e programas de usuário */
#define SYS_PRINT 1
#define SYS_EXIT  2

/* Registra o gate 0x80 na IDT (DPL=3, para o ring 3 poder chamar) */
void syscall_init(void);

/* Roda um programa de usuário já carregado em `entry`, com stack no topo
   `user_stack`. Retorna quando o programa faz sys_exit. */
void exec_user(uint32_t entry, uint32_t user_stack);

#endif
