#include "fb.h"
#include "io.h"

static uint32_t fb_base;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;
static uint8_t  fb_bpp;
static uint8_t *font;

static int      cur_x    = 0;
static int      cur_y    = 0;
static uint32_t color_fg = COLOR_WHITE;
static uint32_t color_bg = COLOR_BG;

/* PCI bus 0: lê dword de (device, register) */
static uint32_t pci_read(uint8_t dev, uint8_t reg) {
    outl(0xCF8, 0x80000000u | ((uint32_t)dev << 11) | (reg & 0xFC));
    return inl(0xCFC);
}

/* Procura BAR0 da placa VGA (classe 0x0300) no bus PCI 0 */
static uint32_t pci_vga_bar0(void) {
    for (uint8_t d = 0; d < 32; d++) {
        if ((pci_read(d, 0) & 0xFFFF) == 0xFFFF) continue;
        if ((pci_read(d, 8) >> 16) == 0x0300) {
            uint32_t bar = pci_read(d, 0x10) & ~0xFu;
            if (bar > 0x100000u) return bar;
        }
    }
    return 0; /* não encontrou */
}

void fb_init(void) {
    /* Bochs VBE foi configurado pelo bootloader via port I/O (0x01CE/0x01CF).
       Parâmetros são fixos: 1024×768×32bpp.
       Endereço do framebuffer = BAR0 da VGA no PCI bus 0 (device 2 no QEMU). */
    fb_width  = 1024;
    fb_height = 768;
    fb_bpp    = 32;
    fb_pitch  = 1024 * 4;  /* 32bpp = 4 bytes/pixel */

    fb_base = pci_vga_bar0();
    if (fb_base < 0x100000u) fb_base = 0xFD000000u;

    font = (uint8_t*)0x6000; /* copiada pelo bootloader (fonte BIOS 8x16) */
}

static void put_pixel(int x, int y, uint32_t rgb) {
    if ((unsigned)x >= fb_width || (unsigned)y >= fb_height) return;
    uint8_t *p = (uint8_t*)(fb_base + (uint32_t)y * fb_pitch +
                             (uint32_t)x * (fb_bpp / 8));
    p[0] = rgb & 0xFF;
    p[1] = (rgb >> 8) & 0xFF;
    p[2] = (rgb >> 16) & 0xFF;
    if (fb_bpp == 32) p[3] = 0;
}

void fb_fill(uint32_t color) {
    uint8_t b = color & 0xFF, g = (color >> 8) & 0xFF, r = (color >> 16) & 0xFF;
    uint8_t bpx = fb_bpp / 8;
    for (uint32_t row = 0; row < fb_height; row++) {
        uint8_t *line = (uint8_t*)(fb_base + row * fb_pitch);
        for (uint32_t col = 0; col < fb_width; col++) {
            uint8_t *p = line + col * bpx;
            p[0] = b; p[1] = g; p[2] = r;
            if (bpx == 4) p[3] = 0;
        }
    }
}

void fb_rect(int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            put_pixel(col, row, color);
}

void fb_char_at(int x, int y, char c, uint32_t fg, uint32_t bg, int scale) {
    uint8_t *glyph = font + (uint8_t)c * 16;
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            uint32_t color = (bits & (0x80 >> col)) ? fg : bg;
            for (int dy = 0; dy < scale; dy++)
                for (int dx = 0; dx < scale; dx++)
                    put_pixel(x + col * scale + dx, y + row * scale + dy, color);
        }
    }
}

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

void fb_print_centered(const char *s, int y, int scale, uint32_t fg) {
    int x = ((int)fb_width - slen(s) * 8 * scale) / 2;
    for (int i = 0; s[i]; i++, x += 8 * scale)
        fb_char_at(x, y, s[i], fg, color_bg, scale);
}

static int margin_x = 0;

void fb_setpos(int x, int y)              { cur_x = x; cur_y = y; margin_x = x; }
int  fb_getx(void)                        { return cur_x; }
int  fb_gety(void)                        { return cur_y; }
void fb_setcolor(uint32_t fg, uint32_t bg) { color_fg = fg; color_bg = bg; }

/* Renderiza array de strings com fonte 8x16.
   Espaços são transparentes: o fundo permanece intacto. */
void fb_art(const char **lines, int x0, int y0, uint32_t fg) {
    for (int i = 0; lines[i] != 0; i++) {
        const char *line = lines[i];
        for (int j = 0; line[j]; j++) {
            if (line[j] == ' ') continue;
            uint8_t *glyph = font + (uint8_t)line[j] * 16;
            for (int row = 0; row < 16; row++) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < 8; col++) {
                    if (bits & (0x80 >> col))
                        put_pixel(x0 + j * 8 + col, y0 + i * 16 + row, fg);
                }
            }
        }
    }
}

/* Sobe todo o conteúdo 16px (uma linha de texto) e limpa a última linha.
   Nota: pixels em 32bpp têm layout B,G,R,0 — um u32 0x00RRGGBB casa direto. */
static void fb_scroll(void) {
    uint32_t *dst = (uint32_t*)fb_base;
    uint32_t *src = (uint32_t*)(fb_base + fb_pitch * 16);
    uint32_t count = (fb_height - 16) * fb_pitch / 4;
    for (uint32_t i = 0; i < count; i++)
        dst[i] = src[i];
    uint32_t *last = (uint32_t*)(fb_base + (fb_height - 16) * fb_pitch);
    for (uint32_t i = 0; i < fb_pitch * 16 / 4; i++)
        last[i] = color_bg;
}

/* Avança uma linha de texto; no fim da tela, faz scroll */
static void newline(void) {
    cur_x = margin_x;
    cur_y += 16;
    if (cur_y + 16 > (int)fb_height) {
        fb_scroll();
        cur_y -= 16;
    }
}

void putchar(char c) {
    if (c == '\n') { newline(); return; }
    if (c == '\b') {
        if (cur_x >= margin_x + 8) cur_x -= 8;
        fb_char_at(cur_x, cur_y, ' ', color_fg, color_bg, 1);
        return;
    }
    fb_char_at(cur_x, cur_y, c, color_fg, color_bg, 1);
    cur_x += 8;
    if (cur_x + 8 > (int)fb_width) newline();
}

void print(const char *s) { for (int i = 0; s[i]; i++) putchar(s[i]); }

void print_color(const char *s, uint32_t color) {
    uint32_t prev = color_fg;
    color_fg = color;
    print(s);
    color_fg = prev;
}

void print_hex(uint32_t n) {
    char buf[11];
    buf[0] = '0'; buf[1] = 'x'; buf[10] = 0;
    for (int i = 9; i >= 2; i--) {
        int d = n & 0xF;
        buf[i] = d < 10 ? '0' + d : 'a' + d - 10;
        n >>= 4;
    }
    print(buf);
}

void clear_screen(void) {
    fb_fill(color_bg);
    cur_x = 0; cur_y = 0;
}
