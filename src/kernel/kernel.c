#include "fb.h"
#include "idt.h"
#include "pic.h"
#include "irq.h"
#include "keyboard.h"
#include "shell.h"

#define SCREEN_W 1024
#define SCREEN_H 768
#define BRAIN_RED 0xFB2C36
#define DARK_RED 0x8B1F27
#define TXT_GRAY 0x999999

/* cursor piscante segue a posição real de digitação (fb_getx/fb_gety) */
static int boot_done = 0;
static int ticks = 0;

static void timer_handler(registers_t *regs)
{
    (void)regs;
    ticks++;
    if (ticks % 27 != 0 || !boot_done)
        return;
    char c = ((ticks / 27) % 2) ? '_' : ' ';
    fb_char_at(fb_getx(), fb_gety(), c, BRAIN_RED, 0x000000, 1);
}

/* usado pelo shell (comando uptime) */
int get_ticks(void)
{
    return ticks;
}

/* brain-ascii.txt — cérebro + circuito, 44 chars × 24 linhas */
static const char *logo[] = {
    "              ++++++++++++++++",
    "          +++++      ##     +##+++",
    "        +#+          ++     #+   +#+",
    "    +++++            ++         ++++++++",
    "   #+      ++++#+    ++   +#  +#+      +#",
    "  ##      +#   ##    ++   +#  #+        ##",
    " +#+       +++++     ++   #+            +#+",
    " +#         +#+      ++  +#+#            #+",
    " +#          #+      ++ +#  +#           #+",
    "+#+  +++++   #+      ++      +#+         +#+",
    "#+  +#+  +#++#+++    ++       +++++++++   +#",
    "#+  +#+++#+    +#    ++          ++++     +#",
    "+#+   +++      +#    ++                  +#+",
    " +#+         ++++++  ++        #+       +#+",
    "   +#+       #+  +#+ ++ ++      +++++++#+",
    "    #+       +#++++  ++  +#+      +++++#",
    "    #+         +#    ++   +#          +#",
    "    +#+       +##+   ++   +##++      +#+",
    "      +#+    +++++#  ++  +#+       +#+",
    "       +#    #+  +#+ ++ +#+        #+",
    "        #++   ++++   ++          ++#",
    "         ++++        ##        ++++",
    "            +#+    ++++++    +#+",
    "              +++++++  +++++++",
    0};

/* 44 cols × 8px = 352 px de largura; 24 linhas × 16px = 384 px de altura */
#define LOGO_W_PX (44 * 8)
#define LOGO_H_PX (24 * 16)

static void ok(const char *msg)
{
    print_color("[  OK  ]   ", COLOR_GREEN);
    print_color(msg, 0xFFFFFF);
    print("\n\n");
}

/* Cantoneiras vermelhas nos 4 cantos da tela */
static void draw_corners(void)
{
    const int in = 14, len = 26, th = 2;
    /* superior esquerdo */
    fb_rect(in, in, len, th, DARK_RED);
    fb_rect(in, in, th, len, DARK_RED);
    /* superior direito */
    fb_rect(SCREEN_W - in - len, in, len, th, DARK_RED);
    fb_rect(SCREEN_W - in - th, in, th, len, DARK_RED);
    /* inferior esquerdo */
    fb_rect(in, SCREEN_H - in - th, len, th, DARK_RED);
    fb_rect(in, SCREEN_H - in - len, th, len, DARK_RED);
    /* inferior direito */
    fb_rect(SCREEN_W - in - len, SCREEN_H - in - th, len, th, DARK_RED);
    fb_rect(SCREEN_W - in - th, SCREEN_H - in - len, th, len, DARK_RED);
}

void kernel_main()
{
    fb_init();
    fb_fill(0x000000);

    draw_corners();

    /* cabeçalho */
    fb_setpos(48, 40);
    fb_setcolor(BRAIN_RED, 0x000000);
    print("Brain OS");
    print_color(" v0.1.0", TXT_GRAY);
    fb_setpos(48, 68);
    print_color("Inicializando sistema...", TXT_GRAY);

    /* [ TTY1 ] no canto superior direito: 8 chars × 8px = 64px */
    fb_setpos(SCREEN_W - 48 - 64, 40);
    print_color("[ TTY1 ]", TXT_GRAY);

    /* logo centralizado */
    fb_art((const char **)logo, (SCREEN_W - LOGO_W_PX) / 2, 88, BRAIN_RED);

    /* mensagens de boot abaixo do logo (88 + 384 = 472) */
    fb_setpos(360, 510);
    fb_setcolor(0xFFFFFF, 0x000000);

    idt_init();
    ok("IDT carregada");

    pic_init();
    ok("PIC inicializado");

    irq_install(0, timer_handler);
    keyboard_init();
    __asm__ volatile("sti");
    ok("Interrupcoes ativas");

    /* separador */
    fb_rect(360, 618, 340, 1, DARK_RED);

    /* prompt */
    fb_setpos(360, 644);
    print_color("> ", BRAIN_RED);
    print_color("sistema pronto. ", TXT_GRAY);

    /* rodapé */
    fb_setpos(48, SCREEN_H - 48);
    print_color("2026 - Syntel", DARK_RED);
    /* direita: 13 chars × 8px = 104px */
    fb_setpos(SCREEN_W - 48 - 104, SCREEN_H - 48);
    print_color("syntel.net.br", DARK_RED);

    /* shell assume a partir da linha abaixo do prompt */
    fb_setpos(360, 676);
    fb_setcolor(0xFFFFFF, 0x000000);
    boot_done = 1;

    shell_run();
}
