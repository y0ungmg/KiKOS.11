#pragma once

#include "types.h"

void heap_init(void);
void *kmalloc(u32 n);
void kfree(void *ptr);
u32 heap_used(void);
u32 heap_total(void);
