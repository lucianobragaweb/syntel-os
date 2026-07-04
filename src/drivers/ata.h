#ifndef ATA_H
#define ATA_H

#include "types.h"

/* Lê `count` setores de 512 bytes a partir do setor `lba`
   (disco primário master, modo PIO) */
void ata_read(uint32_t lba, uint8_t count, void *buf);

#endif
