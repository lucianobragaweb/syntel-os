#include "screen.h"
#include "io.h"

#define VIDEO_MEMORY   0xb8000
#define WHITE_ON_BLACK 0x07
#define RED_ON_BLACK   0x04
#define COLS           80
#define ROWS           25

static int col = 0;
static int row = 0;

static void update_cursor(void) {
    int pos = row * COLS + col;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));
}

static unsigned char current_color = WHITE_ON_BLACK;

static void print_char(char c) {
    char *video = (char*) VIDEO_MEMORY;

    if (c == '\n') {
        col = 0;
        row++;
        update_cursor();
        return;
    }

    if (c == '\b') {
        if (col > 0) col--;
        else if (row > 0) { row--; col = COLS - 1; }
        int offset = (row * COLS + col) * 2;
        video[offset]     = ' ';
        video[offset + 1] = current_color;
        update_cursor();
        return;
    }

    int offset = (row * COLS + col) * 2;
    video[offset]     = c;
    video[offset + 1] = current_color;

    col++;
    if (col >= COLS) {
        col = 0;
        row++;
    }
    update_cursor();
}

void putchar(char c) {
    print_char(c);
}

void print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++)
        print_char(str[i]);
}

void print_color(const char *str, unsigned char color) {
    unsigned char prev = current_color;
    current_color = color;
    print(str);
    current_color = prev;
}

void print_hex(uint32_t n) {
    char buf[11];
    buf[0]  = '0';
    buf[1]  = 'x';
    buf[10] = '\0';

    for (int i = 9; i >= 2; i--) {
        int digit = n & 0xF;
        buf[i] = digit < 10 ? '0' + digit : 'a' + digit - 10;
        n >>= 4;
    }

    print(buf);
}

void clear_screen() {
    char *video = (char*) VIDEO_MEMORY;
    for (int i = 0; i < COLS * ROWS * 2; i += 2) {
        video[i]     = ' ';
        video[i + 1] = WHITE_ON_BLACK;
    }
    col = 0;
    row = 0;
    update_cursor();
}
