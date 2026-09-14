#include "floppy.h"
#include "kio_ports.h"

#define FDC_DOR   0x3F2
#define FDC_MSR   0x3F4
#define FDC_DATA  0x3F5
#define FDC_CCR   0x3F7

#define CMD_SPECIFY       0x03
#define CMD_RECALIBRATE   0x07
#define CMD_SENSE_INT     0x08
#define CMD_READ_SECTOR   0xE6
#define CMD_WRITE_SECTOR  0xC5

static unsigned char dma_buf[512] __attribute__((aligned(4096)));

static void io_wait(void) {
    inb(0x80);
}

static void fdc_write(unsigned char val) {
    int timeout = 10000;
    while (timeout--) {
        if (inb(FDC_MSR) & 0x80) {
            outb(FDC_DATA, val);
            return;
        }
        io_wait();
    }
}

static unsigned char fdc_read(void) {
    int timeout = 10000;
    while (timeout--) {
        if ((inb(FDC_MSR) & 0xC0) == 0xC0) {
            return inb(FDC_DATA);
        }
        io_wait();
    }
    return 0;
}

static void dma_setup(int write) {
    unsigned int addr = (unsigned int)(unsigned long)dma_buf;
    unsigned int count = 511; /* length - 1 */

    outb(0x0A, 0x06);              /* mask channel 2 */
    outb(0x0C, 0xFF);              /* clear flip-flop */
    outb(0x04, (unsigned char)(addr & 0xFF));
    outb(0x04, (unsigned char)((addr >> 8) & 0xFF));
    outb(0x81, (unsigned char)((addr >> 16) & 0xFF)); /* page reg, ch2 */
    outb(0x0C, 0xFF);
    outb(0x05, (unsigned char)(count & 0xFF));
    outb(0x05, (unsigned char)((count >> 8) & 0xFF));
    outb(0x0B, write ? 0x5A : 0x56); /* mode: single, ch2, read/write */
    outb(0x0A, 0x02);               /* unmask channel 2 */
}

static void fdc_sense_interrupt(unsigned char *st0, unsigned char *cyl) {
    fdc_write(CMD_SENSE_INT);
    *st0 = fdc_read();
    *cyl = fdc_read();
}

int floppy_init(void) {
    outb(FDC_DOR, 0x00);
    outb(FDC_DOR, 0x1C);           /* motor on, DMA/IRQ enable, drive 0 */

    fdc_write(CMD_SPECIFY);
    fdc_write(0xDF);               /* SRT/HUT */
    fdc_write(0x02);               /* HLT, non-DMA=0 */

    fdc_write(CMD_RECALIBRATE);
    fdc_write(0x00);               /* drive 0 */

    unsigned char st0, cyl;
    fdc_sense_interrupt(&st0, &cyl);

    return (st0 & 0xC0) == 0 ? 0 : -1;
}

static void lba_to_chs(unsigned int lba, unsigned char *cyl,
                        unsigned char *head, unsigned char *sector) {
    *cyl    = (unsigned char)(lba / (2 * 18));
    *head   = (unsigned char)((lba % (2 * 18)) / 18);
    *sector = (unsigned char)((lba % 18) + 1);
}

int floppy_read_sector(unsigned int lba, unsigned char *buf512) {
    unsigned char cyl, head, sector;
    lba_to_chs(lba, &cyl, &head, &sector);

    dma_setup(0);

    fdc_write(CMD_READ_SECTOR);
    fdc_write((unsigned char)(head << 2));
    fdc_write(cyl);
    fdc_write(head);
    fdc_write(sector);
    fdc_write(2);      /* 512 bytes/sector */
    fdc_write(18);     /* sectors per track */
    fdc_write(0x1B);   /* gap length */
    fdc_write(0xFF);

    for (int i = 0; i < 7; i++) fdc_read(); /* result phase bytes */

    for (int i = 0; i < 512; i++) buf512[i] = dma_buf[i];
    return 0;
}

int floppy_write_sector(unsigned int lba, const unsigned char *buf512) {
    unsigned char cyl, head, sector;
    lba_to_chs(lba, &cyl, &head, &sector);

    for (int i = 0; i < 512; i++) dma_buf[i] = buf512[i];
    dma_setup(1);

    fdc_write(CMD_WRITE_SECTOR);
    fdc_write((unsigned char)(head << 2));
    fdc_write(cyl);
    fdc_write(head);
    fdc_write(sector);
    fdc_write(2);
    fdc_write(18);
    fdc_write(0x1B);
    fdc_write(0xFF);

    for (int i = 0; i < 7; i++) fdc_read();
    return 0;
}