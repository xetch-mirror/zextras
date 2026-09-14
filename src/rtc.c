#include "rtc.h"
#include "kio_ports.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static unsigned char cmos_read(unsigned char reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static int rtc_update_in_progress(void) {
    return cmos_read(0x0A) & 0x80;
}

static unsigned char bcd_to_bin(unsigned char v) {
    return (unsigned char)((v & 0x0F) + ((v / 16) * 10));
}

void rtc_read(rtc_time_t *out) {
    while (rtc_update_in_progress()) { }

    unsigned char second = cmos_read(0x00);
    unsigned char minute = cmos_read(0x02);
    unsigned char hour   = cmos_read(0x04);
    unsigned char day    = cmos_read(0x07);
    unsigned char month  = cmos_read(0x08);
    unsigned char year   = cmos_read(0x09);
    unsigned char regB   = cmos_read(0x0B);

    if (!(regB & 0x04)) { /* BCD mode */
        second = bcd_to_bin(second);
        minute = bcd_to_bin(minute);
        hour   = (unsigned char)(bcd_to_bin(hour & 0x7F) | (hour & 0x80));
        day    = bcd_to_bin(day);
        month  = bcd_to_bin(month);
        year   = bcd_to_bin(year);
    }

    if (!(regB & 0x02) && (hour & 0x80)) { /* 12h mode, PM */
        hour = (unsigned char)(((hour & 0x7F) + 12) % 24);
    }

    out->second = second;
    out->minute = minute;
    out->hour   = hour;
    out->day    = day;
    out->month  = month;
    out->year   = 2000 + year; /* assumes CMOS century byte unused */
}