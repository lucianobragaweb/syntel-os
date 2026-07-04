/* mkfs — grava um filesystem SyFS dentro da imagem de disco.
   Roda no HOST durante o build (como mkfs.ext4 no mundo real).

   uso: mkfs <imagem> <arquivo1> [arquivo2 ...]

   Layout (precisa casar com src/fs/fs.h):
     setor 64   superblock: "SYFS" + numero de arquivos
     setor 65   diretorio: 16 entradas de 32 bytes
     setor 66+  dados, contiguos                                        */
#include <stdio.h>
#include <string.h>

#define FS_START 64
#define MAX_FILES 16
#define SEC 512

struct ent {
    char     name[24];
    unsigned start;
    unsigned size;
};

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "uso: mkfs <imagem> <arquivos...>\n");
        return 1;
    }

    FILE *img = fopen(argv[1], "r+b");
    if (!img) { perror(argv[1]); return 1; }

    int n = argc - 2;
    if (n > MAX_FILES) n = MAX_FILES;

    struct ent dir[MAX_FILES];
    memset(dir, 0, sizeof dir);
    unsigned sector = FS_START + 2;  /* dados começam após superblock+dir */

    for (int i = 0; i < n; i++) {
        const char *path = argv[2 + i];
        const char *base = strrchr(path, '/');
        base = base ? base + 1 : path;

        FILE *f = fopen(path, "rb");
        if (!f) { perror(path); return 1; }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        strncpy(dir[i].name, base, 23);
        dir[i].start = sector;
        dir[i].size  = (unsigned)size;

        fseek(img, (long)sector * SEC, SEEK_SET);
        char buf[SEC];
        long left = size;
        while (left > 0) {
            memset(buf, 0, SEC);
            size_t r = fread(buf, 1, SEC, f);
            if (r == 0) break;
            fwrite(buf, 1, SEC, img);
            left -= (long)r;
        }
        fclose(f);

        sector += (unsigned)((size + SEC - 1) / SEC);
        printf("  %-24s %6ld bytes @ setor %u\n", base, size, dir[i].start);
    }

    /* superblock */
    unsigned char sb[SEC];
    memset(sb, 0, SEC);
    memcpy(sb, "SYFS", 4);
    *(unsigned *)(sb + 4) = (unsigned)n;
    fseek(img, FS_START * SEC, SEEK_SET);
    fwrite(sb, 1, SEC, img);

    /* diretorio */
    fseek(img, (FS_START + 1) * SEC, SEEK_SET);
    fwrite(dir, 1, sizeof dir, img);

    fclose(img);
    printf("SyFS: %d arquivo(s) gravado(s) em %s\n", n, argv[1]);
    return 0;
}
