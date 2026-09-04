#include "pcspk.h"
#include "lib.h"
#include "timer.h"

/* PC speaker via PIT channel 2 (port 0x42/0x43) gated by port 0x61. */

void pcspk_init(void)
{
    /* Ports are standard; nothing to configure. Just make sure the
       speaker is silenced at start so we don't get a stuck tone. */
    u8 t = inb(0x61);
    outb(0x61, t & 0xFC);
}

void beep(u32 freq, u32 ms)
{
    if (!freq) {
        u8 t = inb(0x61);
        outb(0x61, t & 0xFC);
        return;
    }

    u32 div = 1193182 / freq;
    outb(0x43, 0xB6);                      /* ch2, mode 3, binary */
    outb(0x42, (u8)(div & 0xFF));
    outb(0x42, (u8)((div >> 8) & 0xFF));

    u8 tmp = inb(0x61);
    outb(0x61, (u8)(tmp | 0x03));          /* gate speaker + PIT */
    mdelay(ms);
    outb(0x61, (u8)(tmp & 0xFC));          /* silence */
}

void beep_click(void) { beep(660, 10); }
void beep_ok(void)    { beep(523, 70); beep(784, 110); }
void beep_err(void)   { beep(200, 120); beep(150, 160); }
