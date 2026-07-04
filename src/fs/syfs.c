#include "fs.h"
#include "ata.h"

#define FS_MAGIC 0x53465953u  /* "SYFS" em little-endian */

static fs_dirent_t dir[FS_MAX_FILES];  /* diretório em cache na RAM */
static int nfiles = 0;

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

int fs_init(void) {
    uint8_t sb[512];
    ata_read(FS_START_SECTOR, 1, sb);

    if (*(uint32_t *)sb != FS_MAGIC) return -1;  /* disco sem SyFS */

    nfiles = *(uint32_t *)(sb + 4);
    if (nfiles > FS_MAX_FILES) nfiles = FS_MAX_FILES;

    /* diretório: 16 entradas × 32 bytes = exatamente 1 setor */
    ata_read(FS_START_SECTOR + 1, 1, dir);
    return 0;
}

int          fs_count(void)    { return nfiles; }
fs_dirent_t *fs_entry(int i)   { return (i >= 0 && i < nfiles) ? &dir[i] : 0; }

static fs_dirent_t *find(const char *name) {
    for (int i = 0; i < nfiles; i++)
        if (str_eq(dir[i].name, name)) return &dir[i];
    return 0;
}

int fs_stat(const char *name, uint32_t *size) {
    fs_dirent_t *e = find(name);
    if (!e) return -1;
    *size = e->size;
    return 0;
}

int fs_read(const char *name, void *buf, uint32_t max) {
    fs_dirent_t *e = find(name);
    if (!e) return -1;

    uint32_t size = e->size < max ? e->size : max;
    uint32_t remaining = size;
    uint32_t lba = e->start;
    uint8_t *dst = buf;
    uint8_t sector[512];

    /* lê setor a setor: o último pode ser parcial, então copiamos
       só os bytes que pertencem ao arquivo */
    while (remaining > 0) {
        ata_read(lba++, 1, sector);
        uint32_t chunk = remaining > 512 ? 512 : remaining;
        for (uint32_t i = 0; i < chunk; i++)
            dst[i] = sector[i];
        dst += chunk;
        remaining -= chunk;
    }
    return (int)size;
}
