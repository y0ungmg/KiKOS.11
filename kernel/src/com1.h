#pragma once

#include "types.h"

void dbg_init(void);
void dbg_putc(char c);
void dbg(const char *s);
void dbg_hex32(u32 v);
