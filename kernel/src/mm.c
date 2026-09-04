#include "mm.h"
#include "lib.h"

extern u8 _end[];
extern u8 __bss_start[];

static u32 heap_ptr = 0;
#define HEAP_LIMIT 0x40000000u

void heap_init(void)
{
    heap_ptr = ((u32)_end + 15) & ~15u;
}

void *kmalloc(u32 n)
{
    heap_ptr = (heap_ptr + 7) & ~7u;
    if (heap_ptr + n >= HEAP_LIMIT) return 0;
    void *p = (void *)heap_ptr;
    heap_ptr += n;
    memset(p, 0, n);
    return p;
}

u32 heap_used(void)
{
    return heap_ptr - (u32)_end;
}

void kfree(void *ptr)
{
    (void)ptr;
}

u32 heap_total(void)
{
    return HEAP_LIMIT - (u32)_end;
}
