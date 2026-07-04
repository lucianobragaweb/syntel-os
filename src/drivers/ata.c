#include "ata.h"
#include "io.h"
#include "sync.h"

/* ATA PIO, disco primário master — as mesmas portas desde o IBM AT (1984).
   LBA (Logical Block Addressing): setores numerados 0,1,2... linearmente,
   sem a ginástica de cilindro/cabeça/setor do INT 13h do bootloader. */

#define ATA_DATA    0x1F0  /* leitura/escrita de dados, 16 bits por vez */
#define ATA_SECCNT  0x1F2  /* quantos setores                           */
#define ATA_LBA_LO  0x1F3  /* bits 0-7 do LBA                           */
#define ATA_LBA_MID 0x1F4  /* bits 8-15                                 */
#define ATA_LBA_HI  0x1F5  /* bits 16-23                                */
#define ATA_DRIVE   0x1F6  /* selecao de disco + bits 24-27 do LBA      */
#define ATA_CMD     0x1F7  /* escrita: comando / leitura: status        */

#define STATUS_BSY  0x80   /* disco ocupado                             */
#define STATUS_DRQ  0x08   /* dados prontos para transferir             */

#define CMD_READ    0x20

static void wait_bsy(void) { while (inb(ATA_CMD) & STATUS_BSY); }
static void wait_drq(void) { while (!(inb(ATA_CMD) & STATUS_DRQ)); }

void ata_read(uint32_t lba, uint8_t count, void *buf) {
    uint16_t *p = (uint16_t *)buf;
    uint32_t f = irq_save();  /* uma transferência por vez */

    wait_bsy();
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F)); /* 0xE0 = master, modo LBA */
    outb(ATA_SECCNT, count);
    outb(ATA_LBA_LO,  lba        & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8)  & 0xFF);
    outb(ATA_LBA_HI,  (lba >> 16) & 0xFF);
    outb(ATA_CMD, CMD_READ);

    for (int s = 0; s < count; s++) {
        wait_bsy();
        wait_drq();
        for (int i = 0; i < 256; i++)   /* 256 words = 512 bytes */
            *p++ = inw(ATA_DATA);
    }

    irq_restore(f);
}
