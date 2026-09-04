#include "com1.h"
#include "lib.h"

#define COM1 0x3F8

void dbg_init(void)
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void dbg_putc(char c)
{
    u32 t = 100000;
    while (!(inb(COM1 + 5) & 0x20) && --t) ;
    outb(COM1, (u8)c);
}

void dbg(const char *s)
{
    while (*s) dbg_putc(*s++);
}

void dbg_hex32(u32 v)
{
    static const char h[] = "0123456789ABCDEF";
    dbg("0x");
    for (int i = 28; i >= 0; i -= 4)
        dbg_putc(h[(v >> i) & 0xF]);
}
