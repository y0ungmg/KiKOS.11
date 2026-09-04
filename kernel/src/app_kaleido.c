#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "timer.h"

#define N_SEG 12
#define N_LAYERS 7

static u32 tick0;
static u32 pal[N_SEG];

void app_kaleido_open(Window *w)
{
    (void)w;
    rand_seed(g_ticks ^ 0xABCD1234);
    tick0 = g_ticks;
    for (int i = 0; i < N_SEG; i++) {
        u32 r = rand32();
        pal[i] = mixc(g_accent_a, g_accent_b, (u8)(r % 256));
    }
}

void app_kaleido_draw(Window *w, Rect *c)
{
    (void)w;
    blend_rect(c->x, c->y, c->w, c->h, rgb(0, 0, 0), 120);

    int cx = c->x + c->w / 2;
    int cy = c->y + c->h / 2;
    int max_r = c->w < c->h ? c->w / 2 - 16 : c->h / 2 - 16;
    if (max_r < 30) return;

    int seg_deg = 360 / N_SEG;
    int rot = (int)((g_ticks - tick0) * 3) % 360;

    int i, k, deg;

    for (i = 0; i < N_SEG; i++) {
        int px, py;
        arc_point(cx, cy, max_r, rot + i * seg_deg, &px, &py);
        draw_line(cx, cy, px, py, mixc(pal[i], rgb(255, 255, 255), 50));
    }

    for (i = 0; i < N_SEG; i++) {
        int a0 = rot + i * seg_deg + 4;
        int a1 = rot + (i + 1) * seg_deg - 4;
        arc(cx, cy, max_r, a0, a1, pal[i]);
    }

    for (k = 1; k <= 5; k++) {
        int r = max_r * k / 6;
        circle_ring(cx, cy, r, mixc(pal[k % N_SEG], rgb(255, 255, 255), 25));
    }

    for (i = 0; i < N_SEG; i++) {
        int a0 = rot + i * seg_deg + 6;
        int a1 = rot + (i + 1) * seg_deg - 6;
        arc(cx, cy, max_r * 2 / 3, a0, a1, pal[(i + 3) % N_SEG]);
    }

    for (i = 0; i < N_SEG; i++) {
        int a0 = rot + i * seg_deg + 8;
        int a1 = rot + (i + 1) * seg_deg - 8;
        arc(cx, cy, max_r / 2, a0, a1, pal[(i + 6) % N_SEG]);
    }

    for (deg = 0; deg < 360; deg++) {
        int mod = deg % seg_deg;
        int half = seg_deg / 2;
        int mirror = mod <= half ? mod : seg_deg - mod;

        for (k = 0; k < N_LAYERS; k++) {
            int base = max_r * (k + 1) / (N_LAYERS + 2);
            int amp = base / 3;
            int r = base + amp * mirror / half;
            int px1, py1, px2, py2;
            int a = deg + rot;
            arc_point(cx, cy, r > 1 ? r - 1 : 0, a, &px1, &py1);
            arc_point(cx, cy, r + 1, a, &px2, &py2);
            u32 col = pal[(k + deg / seg_deg) % N_SEG];
            draw_line(px1, py1, px2, py2, col);
        }
    }

    for (i = 0; i < N_SEG; i++) {
        for (k = 1; k <= 5; k++) {
            int r = max_r * k / 6;
            int px, py;
            arc_point(cx, cy, r, rot + i * seg_deg, &px, &py);
            circle_ring(px, py, 2, pal[(i + k) % N_SEG]);
        }
    }

    int pulse = 4 + (int)((g_ticks - tick0) % 20) / 3;
    circle_fill(cx, cy, pulse, mixc(g_accent_a, g_accent_b, (u8)((g_ticks * 3) % 256)));
    circle_ring(cx, cy, pulse, rgb(255, 255, 255));
}

void app_kaleido_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w; (void)lx; (void)ly;
    if (ev == ME_RELEASE) {
        rand_seed(g_ticks ^ 0xDEADBEEF);
        for (int i = 0; i < N_SEG; i++) {
            u32 r = rand32();
            pal[i] = mixc(g_accent_a, g_accent_b, (u8)(r % 256));
        }
    }
}

void app_kaleido_key(Window *w, int key)
{
    (void)w; (void)key;
}
