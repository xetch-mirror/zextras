#include "pcspeaker.h"
#include "kio_ports.h"

#define PIT_CH2   0x42
#define PIT_CMD   0x43
#define PC_SPKR   0x61

void speaker_on(unsigned int freq) {
    if (freq == 0) return;

    unsigned int divisor = 1193180 / freq;

    outb(PIT_CMD, 0xB6);              /* channel 2, lobyte/hibyte, square wave */
    outb(PIT_CH2, (unsigned char)(divisor & 0xFF));
    outb(PIT_CH2, (unsigned char)((divisor >> 8) & 0xFF));

    unsigned char tmp = inb(PC_SPKR);
    if (tmp != (tmp | 3)) {
        outb(PC_SPKR, tmp | 3);       /* gate PIT ch2 + enable speaker */
    }
}

void speaker_off(void) {
    unsigned char tmp = inb(PC_SPKR) & 0xFC;
    outb(PC_SPKR, tmp);
}

/* no real timer yet -- caller controls duration via busy loop count */
void speaker_beep(unsigned int freq, unsigned int loops) {
    speaker_on(freq);
    for (volatile unsigned int i = 0; i < loops; i++) {
        for (volatile unsigned int j = 0; j < 100000; j++) { }
    }
    speaker_off();
}