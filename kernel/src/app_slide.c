#include "types.h"
#include "gfx.h"
#include "apps.h"
#include "gui.h"
#include "lib.h"

static int tiles[16];
static int empty_idx;
static int size = 4;

void app_slide_open(Window *w) {
    (void)w;
    for (int i = 0; i < 15; i++) tiles[i] = i + 1;
    tiles[15] = 0;
    empty_idx = 15;
}

static int find_idx(int val) { return -1; }

void app_slide_key(Window *w, int key) {
    (void)w;
    if (key == 'r') {
        for (int i = 0; i < 15; i++) tiles[i] = i + 1;
        tiles[15] = 0;
        empty_idx = 15;
        ui_request_redraw();
    }
}

void app_slide_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    if (ev != ME_PRESS) return;
    int cx = lx / 100;
    int cy = ly / 100;
    int idx = cy * 4 + cx;
    if (idx < 0 || idx > 15) return;
    int dx = (empty_idx % 4) - cx;
    int dy = (empty_idx / 4) - cy;
    if ((dx == 0 && dy == 1) || (dx == 0 && dy == -1) || (dx == 1 && dy == 0) || (dx == -1 && dy == 0)) {
        tiles[empty_idx] = tiles[idx];
        tiles[idx] = 0;
        empty_idx = idx;
        ui_request_redraw();
    }
}

void app_slide_draw(Window *w, Rect *c) {
    (void)c;
    fill_rect(w->r.x, w->r.y, w->r.w, w->r.h, rgb(14, 16, 24));
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = y * size + x;
            int val = tiles[idx];
            if (val == 0) continue;
            int sx = w->r.x + 20 + x * 100;
            int sy = w->r.y + 30 + y * 100;
            round_rect(sx, sy, 90, 90, 12, val == 15 ? g_accent : rgb(30, 35, 48));
            char s[4];
            utoa_dec((u32)val, s);
            text(sx + 45 - text_w(s, 2) / 2, sy + 40, s, 2, rgb(240, 242, 250));
        }
    }
}

void app_slide_close(Window *w) { (void)w; }
