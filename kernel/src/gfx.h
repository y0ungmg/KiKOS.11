#pragma once

#include "types.h"
#include "bootinfo.h"

extern u32 *fb;
extern int SW, SH;
extern int PITCH;

u32 rgb(u8 r, u8 g, u8 b);
u32 mixc(u32 a, u32 b, u8 t);

void gfx_init(BootInfo *bi);
void putpx(int x, int y, u32 c);
u32  getpx(int x, int y);
void fill_rect(int x, int y, int w, int h, u32 c);
void rect_outline(int x, int y, int w, int h, u32 c);
void blend_rect(int x, int y, int w, int h, u32 c, u8 a);
void round_rect(int x, int y, int w, int h, int r, u32 c);
void round_rect_blend(int x, int y, int w, int h, int r, u32 c, u8 a);
void gfx_row_inset(int j, int h, int r, int *inset);
void hline(int x, int y, int w, u32 c);
void vline(int x, int y, int h, u32 c);
void draw_line(int x0, int y0, int x1, int y1, u32 c);
void circle_fill(int cx, int cy, int r, u32 c);
void circle_ring(int cx, int cy, int r, u32 c);
void arc(int cx, int cy, int r, int a0deg, int a1deg, u32 c);
void arc_point(int cx, int cy, int r, int deg, int *px, int *py);

void text(int x, int y, const char *s, int scale, u32 c);
void text_blend(int x, int y, const char *s, int scale, u32 c, u8 a);
int  text_w(const char *s, int scale);
int  font_h(void);

void wall_render(int theme);
void wall_blit(void);

extern const u8 font8x8[96][8];
