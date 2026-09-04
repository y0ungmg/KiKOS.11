#pragma once

#include "types.h"

void mouse_init(void);
void mouse_set_bounds(int w, int h);
void irq_mouse_handler(void);

extern int ms_x, ms_y;
extern int ms_btn_l, ms_btn_r;
extern volatile u8 ms_moved;
extern volatile u8 ms_state_dirty;
