#include "types.h"
#include "gfx.h"
#include "apps.h"
#include "lib.h"
#include "gui.h"
#include "timer.h"

#define GOL_W 120
#define GOL_H 80
#define CELL 4

static u8 grid[2][GOL_W * GOL_H];
static int cur = 0;
static u32 last_tick = 0;
static int running = 1;
static int show_grid = 1;

static int idx(int x, int y) {
    if (x < 0) x += GOL_W;
    if (x >= GOL_W) x -= GOL_W;
    if (y < 0) y += GOL_H;
    if (y >= GOL_H) y -= GOL_H;
    return y * GOL_W + x;
}

static int count_neighbors(int x, int y) {
    int c = 0;
    u8 *g = grid[cur];
    c += g[idx(x-1, y-1)];
    c += g[idx(x,   y-1)];
    c += g[idx(x+1, y-1)];
    c += g[idx(x-1, y)];
    c += g[idx(x+1, y)];
    c += g[idx(x-1, y+1)];
    c += g[idx(x,   y+1)];
    c += g[idx(x+1, y+1)];
    return c;
}

static void step(void) {
    u8 *src = grid[cur];
    u8 *dst = grid[1 - cur];
    for (int y = 0; y < GOL_H; y++) {
        for (int x = 0; x < GOL_W; x++) {
            int n = count_neighbors(x, y);
            int i = y * GOL_W + x;
            if (src[i]) {
                dst[i] = (n == 2 || n == 3) ? 1 : 0;
            } else {
                dst[i] = (n == 3) ? 1 : 0;
            }
        }
    }
    cur = 1 - cur;
}

static void randomize(void) {
    u8 *g = grid[cur];
    for (int i = 0; i < GOL_W * GOL_H; i++) {
        g[i] = (rand32() & 3) == 0;
    }
}

static void clear_grid(void) {
    u8 *g = grid[cur];
    for (int i = 0; i < GOL_W * GOL_H; i++) g[i] = 0;
}

static void glider(void) {
    clear_grid();
    u8 *g = grid[cur];
    g[idx(10, 10)] = 1;
    g[idx(11, 11)] = 1;
    g[idx(9, 12)] = 1;
    g[idx(10, 12)] = 1;
    g[idx(11, 12)] = 1;
}

void app_gol_open(Window *w) {
    (void)w;
    randomize();
    last_tick = g_ticks;
    running = 1;
}

void app_gol_key(Window *w, int key) {
    (void)w;
    if (key == ' ' || key == 'p') running = !running;
    else if (key == 'r') randomize();
    else if (key == 'c') clear_grid();
    else if (key == 'g') glider();
    else if (key == 'h') show_grid = !show_grid;
    else if (key == 17) { /* Ctrl+R - faster */ }
    else if (key == 18) { /* Ctrl+S - slower */ }
}

void app_gol_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    if (ev == ME_PRESS && running == 0) {
        int cx = lx / CELL;
        int cy = ly / CELL;
        if (cx >= 0 && cx < GOL_W && cy >= 0 && cy < GOL_H) {
            u8 *g = grid[cur];
            g[idx(cx, cy)] ^= 1;
            ui_request_redraw();
        }
    }
}

void app_gol_draw(Window *w, Rect *c) {
    (void)c;
    Rect content = win_content(w);
    fill_rect(content.x, content.y, content.w, content.h, rgb(10, 12, 18));

    u32 now = g_ticks;
    if (running && now - last_tick >= 3) {
        step();
        last_tick = now;
    }

    u8 *g = grid[cur];
    u32 alive_color = mixc(g_accent, rgb(255, 255, 255), 30);
    u32 dead_color = rgb(18, 20, 28);

    for (int y = 0; y < GOL_H; y++) {
        int sy = content.y + y * CELL;
        for (int x = 0; x < GOL_W; x++) {
            int sx = content.x + x * CELL;
            if (g[idx(x, y)]) {
                fill_rect(sx, sy, CELL, CELL, alive_color);
            } else {
                fill_rect(sx, sy, CELL, CELL, dead_color);
            }
        }
    }

    if (show_grid) {
        u32 line_c = rgb(22, 24, 34);
        for (int x = 0; x <= content.w; x += CELL) {
            vline(content.x + x, content.y, content.h, line_c);
        }
        for (int y = 0; y <= content.h; y += CELL) {
            hline(content.x, content.y + y, content.w, line_c);
        }
    }

    int tx = content.x + 8;
    int ty = content.y + 8;
    text(tx, ty, "Conway's Game of Life", 1, g_accent);
    text(tx, ty + 14, "SPACE: pause  R: random  C: clear  G: glider  H: grid", 1, rgb(120, 128, 140));
    text(tx, ty + 28, running ? "Running..." : "Paused", 1, running ? rgb(80, 220, 120) : rgb(235, 90, 100));
}

void app_gol_close(Window *w) { (void)w; }