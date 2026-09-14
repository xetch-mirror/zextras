#include "ps2mouse.h"
#include "kio_ports.h"

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD  0x64

static unsigned char cycle = 0;
static signed char packet_bytes[3];
static mouse_packet_t latest;
static int packet_ready = 0;

static void wait_input(void) {
    int timeout = 100000;
    while (timeout-- && (inb(PS2_STATUS) & 0x02)) { }
}

static void wait_output(void) {
    int timeout = 100000;
    while (timeout-- && !(inb(PS2_STATUS) & 0x01)) { }
}

static void mouse_write(unsigned char val) {
    wait_input();
    outb(PS2_CMD, 0xD4);
    wait_input();
    outb(PS2_DATA, val);
}

static unsigned char mouse_read(void) {
    wait_output();
    return inb(PS2_DATA);
}

void mouse_init(void) {
    wait_input();
    outb(PS2_CMD, 0xA8);          /* enable aux device */

    wait_input();
    outb(PS2_CMD, 0x20);          /* get controller config byte */
    wait_output();
    unsigned char status = inb(PS2_DATA) | 0x02; /* enable IRQ12 */
    status &= ~0x20;              /* enable mouse clock */

    wait_input();
    outb(PS2_CMD, 0x60);
    wait_input();
    outb(PS2_DATA, status);

    mouse_write(0xF6);            /* set defaults */
    mouse_read();                 /* ack */

    mouse_write(0xF4);            /* enable data reporting */
    mouse_read();                 /* ack */

    cycle = 0;
    packet_ready = 0;
}

/* call this from the IRQ12 ISR after masking/EOI is handled elsewhere */
void mouse_irq_handler(void) {
    unsigned char data = inb(PS2_DATA);

    switch (cycle) {
        case 0:
            if (!(data & 0x08)) return; /* sync bit not set, drop byte */
            packet_bytes[0] = (signed char)data;
            cycle = 1;
            break;
        case 1:
            packet_bytes[1] = (signed char)data;
            cycle = 2;
            break;
        case 2:
            packet_bytes[2] = (signed char)data;
            cycle = 0;

            latest.left   = packet_bytes[0] & 0x01;
            latest.right  = (packet_bytes[0] & 0x02) >> 1;
            latest.middle = (packet_bytes[0] & 0x04) >> 2;
            latest.dx     = packet_bytes[1];
            latest.dy     = packet_bytes[2];
            packet_ready  = 1;
            break;
    }
}

int mouse_poll(mouse_packet_t *out) {
    if (!packet_ready) return 0;
    *out = latest;
    packet_ready = 0;
    return 1;
}