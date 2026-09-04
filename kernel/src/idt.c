#include "idt.h"
#include "lib.h"

typedef struct {
    u16 lo;
    u16 sel;
    u8  zero;
    u8  flags;
    u16 hi;
} __attribute__((packed)) Gate;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) IdtPtr;

static Gate idt[256];
static IdtPtr idtp;

extern void *isr_stub_table[256];

void idt_set_gate(int n, u32 handler)
{
    idt[n].lo = handler & 0xFFFF;
    idt[n].sel = 0x08;
    idt[n].zero = 0;
    idt[n].flags = 0x8E;
    idt[n].hi = (handler >> 16) & 0xFFFF;
}

void pic_remap(void)
{
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    irq_mask_set(0xFF, 0xFF);
}

void irq_mask_set(u8 master, u8 slave)
{
    outb(0x21, master);
    outb(0xA1, slave);
}

void irq_eoi(u32 irq)
{
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void idt_init(void)
{
    for (int i = 0; i < 256; i++)
        idt_set_gate(i, (u32)isr_stub_table[i]);
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (u32)&idt;
    __asm__ volatile("lidt %0" :: "m"(idtp));
}
