#ifndef ZEXTRAS_PS2MOUSE_H
#define ZEXTRAS_PS2MOUSE_H

typedef struct {
    int left, right, middle;
    signed char dx, dy;
} mouse_packet_t;

void mouse_init(void);
void mouse_irq_handler(void);   /* call from your IRQ12 stub */
int  mouse_poll(mouse_packet_t *out); /* returns 1 if new packet ready */

#endif