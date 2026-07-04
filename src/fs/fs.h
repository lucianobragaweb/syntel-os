#ifndef FS_H
#define FS_H

#include "types.h"

/* SyFS — filesystem read-only mínimo do syntel-os.
   Layout no disco (setores de 512 bytes):
     setor 64   superblock: magic "SYFS" (u32) + numero de arquivos (u32)
     setor 65   diretório: 16 entradas de 32 bytes
     setor 66+  dados, contíguos por arquivo */

#define FS_START_SECTOR 64
#define FS_MAX_FILES    16

typedef struct {
    char     name[24];  /* nome, terminado em 0        */
    uint32_t start;     /* primeiro setor (LBA)        */
    uint32_t size;      /* tamanho em bytes            */
} __attribute__((packed)) fs_dirent_t;

/* Lê superblock e diretório do disco. Retorna 0 se montou, -1 se não */
int fs_init(void);

/* Listagem */
int          fs_count(void);
fs_dirent_t *fs_entry(int i);

/* Tamanho de um arquivo por nome; -1 se não existe */
int fs_stat(const char *name, uint32_t *size);

/* Copia o conteúdo para `buf` (até `max` bytes); retorna bytes lidos ou -1 */
int fs_read(const char *name, void *buf, uint32_t max);

#endif
