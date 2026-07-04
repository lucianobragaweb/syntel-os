/* init — primeiro programa de usuário do brain-os.
   Compilado e linkado SEPARADO do kernel (base 0x400000), roda em ring 3.
   Não tem acesso ao kernel: só consegue agir através de syscalls. */

static void sys_print(const char *s) {
    __asm__ volatile("int $0x80" : : "a"(1), "b"(s));  /* SYS_PRINT */
}

static void sys_exit(void) {
    __asm__ volatile("int $0x80" : : "a"(2));           /* SYS_EXIT */
}

/* _start fica no início do binário (.text.start) — é para cá que o
   kernel salta ao carregar o programa em 0x400000. */
__attribute__((section(".text.start")))
void _start(void) {
    sys_print("  Ola! Sou o init, um programa em ring 3.\n");
    sys_print("  Nao faco parte do kernel: fui carregado do disco e so\n");
    sys_print("  consigo escrever na tela pedindo ao kernel via syscall.\n");
    sys_exit();
}
