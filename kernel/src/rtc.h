#pragma once

#include "types.h"

typedef struct {
    u8 sec, min, hour;
    u8 day, mon;
    u16 year;
} RtcTime;

extern RtcTime g_rtc;

void rtc_init(void);
void rtc_poll(void);
void format_time(char *buf, u32 n);
void format_date(char *buf, u32 n);
