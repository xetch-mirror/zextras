#ifndef ZEXTRAS_RTC_H
#define ZEXTRAS_RTC_H

typedef struct {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int  year;
} rtc_time_t;

void rtc_read(rtc_time_t *out);

#endif