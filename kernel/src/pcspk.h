#pragma once

#include "types.h"

void pcspk_init(void);
void beep(u32 freq, u32 ms);
void beep_click(void);
void beep_ok(void);
void beep_err(void);
