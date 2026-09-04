#include "rtc.h"
#include "lib.h"
#include "timer.h"
#include "gui.h"

RtcTime g_rtc = { 0, 0, 12, 1, 1, 2026 };

static u8 cmos_read(u8 reg)
{
    outb(0x70, reg);
    io_wait();
    return inb(0x71);
}

static int update_in_progress(void)
{
    outb(0x70, 0x0A);
    return inb(0x71) & 0x80;
}

static u8 bcd(u8 v)
{
    return (u8)((v & 0x0F) + ((v >> 4) * 10));
}

static void read_all(void)
{
    while (update_in_progress()) ;

    g_rtc.sec  = cmos_read(0x00);
    g_rtc.min  = cmos_read(0x02);
    g_rtc.hour = cmos_read(0x04);
    g_rtc.day  = cmos_read(0x07);
    g_rtc.mon  = cmos_read(0x08);
    g_rtc.year = cmos_read(0x09);

    u8 regb = cmos_read(0x0B);

    if (!(regb & 0x04)) {
        g_rtc.sec  = bcd(g_rtc.sec);
        g_rtc.min  = bcd(g_rtc.min);
        g_rtc.hour = (u8)(bcd((u8)(g_rtc.hour & 0x7F)) | (g_rtc.hour & 0x80));
        g_rtc.day  = bcd(g_rtc.day);
        g_rtc.mon  = bcd(g_rtc.mon);
        g_rtc.year = bcd(g_rtc.year);
    }

    if (!(regb & 0x02)) {
        u8 pm = (u8)(g_rtc.hour & 0x80);
        g_rtc.hour &= 0x7F;
        if (pm && g_rtc.hour != 12) g_rtc.hour += 12;
        else if (!pm && g_rtc.hour == 12) g_rtc.hour = 0;
    }

    g_rtc.year += 2000;
}

void rtc_init(void)
{
    read_all();
}

void rtc_poll(void)
{
    static u32 last_sec_tick = 0;
    if (g_ticks - last_sec_tick >= 100) {
        last_sec_tick = g_ticks;
        read_all();
    }
}

void format_time(char *buf, u32 n)
{
    (void)n;
    char t[4];
    buf[0] = 0;
    if (g_clock24) {
        if (g_rtc.hour < 10) strcpy(buf, "0");
        utoa_dec(g_rtc.hour, t); strcat(buf, t);
    } else {
        u8 h = (u8)(g_rtc.hour % 12);
        if (!h) h = 12;
        utoa_dec(h, t);
        strcat(buf, t);
    }
    strcat(buf, ":");
    if (g_rtc.min < 10) strcat(buf, "0");
    utoa_dec(g_rtc.min, t);
    strcat(buf, t);
    strcat(buf, ":");
    if (g_rtc.sec < 10) strcat(buf, "0");
    utoa_dec(g_rtc.sec, t);
    strcat(buf, t);
    if (!g_clock24) strcat(buf, g_rtc.hour >= 12 ? " PM" : " AM");
}

void format_date(char *buf, u32 n)
{
    (void)n;
    char t[6];
    buf[0] = 0;
    if (g_rtc.day < 10) strcat(buf, "0");
    utoa_dec(g_rtc.day, t); strcat(buf, t);
    strcat(buf, ".");
    if (g_rtc.mon < 10) strcat(buf, "0");
    utoa_dec(g_rtc.mon, t); strcat(buf, t);
    strcat(buf, ".");
    utoa_dec(g_rtc.year, t); strcat(buf, t);
}
