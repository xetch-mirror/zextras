#ifndef ZEXTRAS_FLOPPY_H
#define ZEXTRAS_FLOPPY_H

#include <stddef.h>

int floppy_init(void);
int floppy_read_sector(unsigned int lba, unsigned char *buf512);
int floppy_write_sector(unsigned int lba, const unsigned char *buf512);

#endif