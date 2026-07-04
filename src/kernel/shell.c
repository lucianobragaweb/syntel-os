#include "shell.h"
#include "fb.h"
#include "keyboard.h"
#include "memory.h"
#include "task.h"
#include "fs.h"

#define PROMPT_RED 0xFB2C36
#define TXT_GRAY   0x999999
#define LINE_MAX   128

/* definido em kernel.c — contador do timer (PIT, ~18.2 Hz) */
int get_ticks(void);

/* ---- helpers de string (ambiente freestanding: sem libc) ---- */

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

/* se `s` começa com `prefix`, retorna ponteiro para o resto; senão 0 */
static const char *skip_prefix(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return s;
}

static void print_dec(uint32_t n) {
    char buf[11];
    int i = 10;
    buf[i] = 0;
    do { buf[--i] = '0' + n % 10; n /= 10; } while (n);
    print(&buf[i]);
}

/* ---- comandos ---- */

static void cmd_help(void) {
    print_color("comandos disponiveis:\n", TXT_GRAY);
    print("  help    - esta lista\n");
    print("  clear   - limpa a tela\n");
    print("  echo X  - imprime X\n");
    print("  uptime  - tempo desde o boot\n");
    print("  meminfo - mapa de memoria e heap\n");
    print("  pagefault - acessa memoria nao mapeada (teste do paging)\n");
    print("  ps      - lista as tarefas\n");
    print("  ls      - lista os arquivos do disco\n");
    print("  cat X   - mostra o conteudo do arquivo X\n");
}

static void cmd_ls(void) {
    if (fs_count() == 0) {
        print_color("nenhum arquivo (filesystem nao montado?)\n", 0xFF5555);
        return;
    }
    print_color("  TAMANHO  NOME\n", TXT_GRAY);
    for (int i = 0; i < fs_count(); i++) {
        fs_dirent_t *e = fs_entry(i);
        print("  ");
        print_dec(e->size);
        print("  ");
        print(e->name);
        print("\n");
    }
}

static void cmd_cat(const char *name) {
    uint32_t size;
    if (fs_stat(name, &size) < 0) {
        print_color("arquivo nao encontrado: ", 0xFF5555);
        print(name);
        print("\n");
        return;
    }
    char *buf = kmalloc(size + 1);
    if (!buf) { print_color("sem memoria\n", 0xFF5555); return; }
    fs_read(name, buf, size);
    buf[size] = 0;
    print(buf);
    if (size > 0 && buf[size - 1] != '\n') print("\n");
}

static void cmd_ps(void) {
    print_color("  ID  NOME\n", TXT_GRAY);
    for (int i = 0; i < MAX_TASKS; i++) {
        const char *name = task_name(i);
        if (!name) continue;
        print("  ");
        print_dec((uint32_t)i);
        print("   ");
        print(name);
        if (i == task_current()) print_color("   <- executando agora", 0x77DD77);
        print("\n");
    }
}

static void cmd_meminfo(void) {
    print_color("regioes reportadas pelo BIOS (E820):\n", TXT_GRAY);
    for (uint32_t i = 0; i < e820_count(); i++) {
        e820_entry_t *e = e820_entry(i);
        print("  ");
        print_hex((uint32_t)e->base);
        print(" - ");
        print_hex((uint32_t)(e->base + e->len));
        print(e->type == E820_USABLE ? "  RAM\n" : "  reservado\n");
    }
    print("total utilizavel: ");
    print_dec(memory_total() / (1024 * 1024));
    print(" MB\n");
    print("heap do kernel:   ");
    print_dec(heap_used());
    print(" / ");
    print_dec(heap_size() / (1024 * 1024));
    print(" MB usados\n");
}

static void cmd_uptime(void) {
    uint32_t t = (uint32_t)get_ticks();
    print("uptime: ");
    print_dec(t / 18);          /* PIT dispara ~18.2x por segundo */
    print("s (");
    print_dec(t);
    print(" ticks)\n");
}

static void execute(const char *line) {
    const char *arg;

    if (line[0] == 0)                return;
    if (str_eq(line, "help"))        { cmd_help();    return; }
    if (str_eq(line, "uptime"))      { cmd_uptime();  return; }
    if (str_eq(line, "meminfo"))     { cmd_meminfo(); return; }
    if (str_eq(line, "ps"))          { cmd_ps();      return; }
    if (str_eq(line, "ls"))          { cmd_ls();      return; }
    if ((arg = skip_prefix(line, "cat "))) { cmd_cat(arg); return; }
    if (str_eq(line, "pagefault")) {
        print("escrevendo em 0x20000000 (nao mapeado)...\n");
        *(volatile uint32_t *)0x20000000 = 42;  /* MMU deve barrar */
        print("se voce le isso, o paging NAO esta funcionando!\n");
        return;
    }
    if (str_eq(line, "clear")) {
        clear_screen();
        fb_setpos(8, 8);   /* console ocupa a tela toda a partir daqui */
        return;
    }
    if ((arg = skip_prefix(line, "echo "))) {
        print(arg);
        print("\n");
        return;
    }

    print_color("comando nao encontrado: ", 0xFF5555);
    print(line);
    print("\n");
}

/* ---- leitura de linha (consumidor do buffer do teclado) ---- */

static void read_line(char *buf) {
    int n = 0;
    for (;;) {
        char c = keyboard_getchar();  /* bloqueia (hlt) ate ter tecla */

        /* apaga o cursor piscante antes de alterar a tela */
        fb_char_at(fb_getx(), fb_gety(), ' ', 0x000000, 0x000000, 1);

        if (c == '\n') { putchar('\n'); break; }
        if (c == '\b') {
            if (n > 0) { n--; putchar('\b'); }
            continue;
        }
        if (n < LINE_MAX - 1) {
            buf[n++] = c;
            putchar(c);
        }
    }
    buf[n] = 0;
}

void shell_run(void) {
    int first = 1;
    for (;;) {
        char line[LINE_MAX];
        print_color("> ", PROMPT_RED);
        read_line(line);

        /* primeiro comando: sai da splash para um console limpo,
           repetindo a linha digitada no topo para manter o contexto */
        if (first && line[0]) {
            first = 0;
            clear_screen();
            fb_setpos(8, 8);
            print_color("> ", PROMPT_RED);
            print(line);
            print("\n");
        }

        execute(line);
    }
}
