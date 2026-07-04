#ifndef FB_H
#define FB_H

#include "types.h"

/* Cores RGB */
#define COLOR_WHITE   0xFFFFFF
#define COLOR_GRAY    0xAAAAAA
#define COLOR_DGRAY   0x555555
#define COLOR_RED     0xFF5555
#define COLOR_GREEN   0x77DD77
#define COLOR_ORANGE  0xE95420
#define COLOR_PURPLE  0xAA7EC5
#define COLOR_BG      0x000000

/* Inicializa lendo info VBE de 0x7000 e fonte de 0x7200 */
void fb_init(void);

/* Primitivas */
void fb_fill(uint32_t color);
void fb_rect(int x, int y, int w, int h, uint32_t color);
void fb_char_at(int x, int y, char c, uint32_t fg, uint32_t bg, int scale);
void fb_print_centered(const char *s, int y, int scale, uint32_t fg);
void fb_setpos(int x, int y);
int  fb_getx(void);
int  fb_gety(void);
uint32_t fb_get_base(void);  /* endereco fisico do framebuffer */
void fb_setcolor(uint32_t fg, uint32_t bg);
/* Renderiza array de strings em modo compacto 8x8px por char */
void fb_art(const char **lines, int x0, int y0, uint32_t fg);

/* API compatível com screen.h */
void putchar(char c);
void print(const char *s);
void print_color(const char *s, uint32_t color);
void print_hex(uint32_t n);
void clear_screen(void);

#endif
