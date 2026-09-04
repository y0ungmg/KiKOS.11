#pragma once

#include "types.h"

void kbd_init(void);
int  kbd_pop(void);
void irq_kbd_handler(void);
