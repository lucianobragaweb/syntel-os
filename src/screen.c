#define VIDEO_MEMORY 0xb8000
#define WHITE_ON_BLACK 0x07

int cursor = 0;

void print_char(char c) {
    char *video = (char*) VIDEO_MEMORY;

    video[cursor * 2] = c;
    video[cursor * 2 + 1] = WHITE_ON_BLACK;

    cursor++;
}

void print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}
