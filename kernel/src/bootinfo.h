#pragma once

#include "types.h"

typedef struct {
    u32 fb_addr;
    u32 fb_pitch;
    u32 fb_width;
    u32 fb_height;
    u32 fb_bpp;
    u32 ext_mem_kb;
    u32 mem_count;
    u32 mem_map;
    u32 boot_drive;
} BootInfo;
