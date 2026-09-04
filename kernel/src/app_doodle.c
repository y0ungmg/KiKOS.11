#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "mm.h"
#include "fs.h"

#define CANVAS_W 500
#define CANVAS_H 320

static u32 *canvas = 0;
static int drawing = 0;
static int last_px, last_py;
static u32 brush = 0;
static const u32 palette[] = {
    0xF2F4F8, 0x35E0DA, 0xB46BFA, 0xFF8C5A,
    0xFFD24E, 0x6BE26B, 0x5AA0FF, 0xE84A6A,
};
#define NPAL 8

void app_doodle_init(void)
{
    if (!canvas) canvas = (u32 *)kmalloc(CANVAS_W * CANVAS_H * 4);
    if (!canvas) return;
    for (int j = 0; j < CANVAS_H; j++)
        for (int i = 0; i < CANVAS_W; i++) {
            u8 t = (u8)((i + j) * 60 / (CANVAS_W + CANVAS_H));
            canvas[(u32)j * CANVAS_W + i] = rgb(30 - t / 3, 33 - t / 3, 44 - t / 3);
        }
}

static void plot(int x, int y)
{
    if (!canvas) return;
    if ((unsigned)x >= CANVAS_W || (unsigned)y >= CANVAS_H) return;
    canvas[(u32)y * CANVAS_W + x] =
        brush == 0 ? rgb(30, 33, 44) : palette[brush - 1];
}

void app_doodle_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w;

    if (ev == ME_PRESS) {
        for (int i = 0; i < NPAL; i++) {
            Rect sr = { 58 + i * 26, 7, 20, 20 };
            if (ui_in(sr, lx, ly)) { brush = (u32)(i + 1); ui_request_redraw(); return; }
        }
        Rect er = { win_content(w).w - 74, 6, 62, 22 };
        Rect wr = { 0, 0, 0, 0 };
        (void)wr;
        if (ui_in(er, lx, ly)) { app_doodle_init(); ui_request_redraw(); return; }
    }

    Rect cv = { 10, 34, CANVAS_W, CANVAS_H };
    int in_canvas = ui_in(cv, lx, ly);

    if (ev == ME_PRESS && in_canvas) {
        drawing = 1;
        last_px = lx - cv.x;
        last_py = ly - cv.y;
        plot(last_px, last_py);
        ui_request_redraw();
    } else if (ev == ME_RELEASE) {
        drawing = 0;
    } else if (ev == ME_MOVE && drawing) {
        int px = lx - cv.x, py = ly - cv.y;
        for (int t = 0; t <= 16; t++) {
            int ix = last_px + (px - last_px) * t / 16;
            int iy = last_py + (py - last_py) * t / 16;
            plot(ix, iy);
            plot(ix + 1, iy);
            plot(ix, iy + 1);
        }
        last_px = px;
        last_py = py;
        ui_request_redraw();
    }
}

void app_doodle_draw(Window *w, Rect *c)
{
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(19, 21, 29));

    text(c->x + 10, c->y + 12, "brush", 1, rgb(140, 146, 160));
    for (int i = 0; i < NPAL; i++) {
        Rect sr = { c->x + 58 + i * 26, c->y + 7, 20, 20 };
        round_rect(sr.x, sr.y, sr.w, sr.h, 6, palette[i]);
        rect_outline(sr.x, sr.y, sr.w + 1, sr.h + 1,
                     (int)brush == i + 1 ? g_accent : rgb(70, 76, 92));
        if (ui_in(sr, ms_x, ms_y))
            blend_rect(sr.x, sr.y, sr.w, sr.h, rgb(255, 255, 255), 30);
    }

    Rect er = { c->x + c->w - 74, c->y + 6, 62, 22 };
    ui_button(er, "clear");

    Rect cvr = { c->x + 9, c->y + 33, CANVAS_W + 2, CANVAS_H + 2 };
    rect_outline(cvr.x, cvr.y, cvr.w, cvr.h, rgb(64, 70, 88));

    if (canvas) {
        for (int j = 0; j < CANVAS_H; j++)
            memcpy(fb + (u32)(cvr.y + 1 + j) * PITCH + cvr.x + 1,
                   canvas + (u32)j * CANVAS_W, CANVAS_W * 4);
    }
}
