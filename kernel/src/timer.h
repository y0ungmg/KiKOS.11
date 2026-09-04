#pragma once

#include "types.h"

extern volatile u32 g_ticks;

void timer_init(u32 hz);
void sleep_ticks(u32 t);
u32  uptime_ms(void);
void irq_timer_handler(void);
void udelay(u32 us);
void mdelay(u32 ms);
