#pragma once

#include "types.h"

void idt_init(void);
void isr_dispatch(u32 n);
void irq_mask_set(u8 master, u8 slave);
void irq_eoi(u32 irq);
void pic_remap(void);
