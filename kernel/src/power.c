#include "power.h"
#include "lib.h"
#include "idt.h"

void power_off(void)
{
    irq_mask_set(0xFF, 0xFF);
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
    outw(0x728, 0x2000);
    __asm__ volatile("cli; hlt");
    for (;;) ;
}

void reboot(void)
{
    u32 t = 100000;
    while (--t && (inb(0x64) & 2)) ;
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("hlt");
}
