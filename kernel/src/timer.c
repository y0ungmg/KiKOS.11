#include "timer.h"
#include "lib.h"
#include "idt.h"

volatile u32 g_ticks = 0;

void irq_timer_handler(void)
{
    g_ticks++;
    irq_eoi(0);
}

void timer_init(u32 hz)
{
    u32 div = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, div & 0xFF);
    outb(0x40, (div >> 8) & 0xFF);
    irq_mask_set(0xFF, 0xFF);  /* stay masked until entry unmasks IRQ0 */
}

void sleep_ticks(u32 t)
{
    u32 end = g_ticks + t;
    while ((i32)(g_ticks - end) < 0) __asm__ volatile("hlt");
}

u32 uptime_ms(void)
{
    return g_ticks * 10;
}

void udelay(u32 us)
{
    u32 end = g_ticks + (us + 9999) / 10000;
    while ((i32)(g_ticks - end) < 0) __asm__ volatile("hlt");
}

void mdelay(u32 ms)
{
    u32 end = g_ticks + ms / 10;
    while ((i32)(g_ticks - end) < 0) __asm__ volatile("hlt");
}
