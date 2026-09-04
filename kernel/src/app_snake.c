#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "timer.h"
#include "pcspk.h"

/* A small grid-based Snake game. Movement is ticked inside the draw
   callback so it keeps animating even when the window is unfocused. */

#define SX 22
#define SY 16

typedef struct { int x, y; } Pt;

static Pt   snake[SX * SY];
static int  snk_len;
static int  dirx, diry;
static Pt   food;
static int  score;
static int  dead;
static int  paused;
static u32  last_move;

static void place_food(void)
{
    for (int tries = 0; tries < 2000; tries++) {
        int fx = (int)(rand32() % SX);
        int fy = (int)(rand32() % SY);
        int ok = 1;
        for (int i = 0; i < snk_len; i++)
            if (snake[i].x == fx && snake[i].y == fy) { ok = 0; break; }
        if (ok) { food.x = fx; food.y = fy; return; }
    }
    food.x = 0; food.y = 0;
}

static void snake_reset(void)
{
    snk_len = 3;
    snake[0].x = SX / 2;     snake[0].y = SY / 2;
    snake[1].x = SX / 2 - 1; snake[1].y = SY / 2;
    snake[2].x = SX / 2 - 2; snake[2].y = SY / 2;
    dirx = 1; diry = 0;
    score = 0; dead = 0; paused = 0;
    last_move = g_ticks;
    place_food();
}

void app_snake_open(Window *w)
{
    (void)w;
    rand_seed(g_ticks ^ 0x1234BEEF);
    snake_reset();
}

void app_snake_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w; (void)lx; (void)ly; (void)ev;
    if (ev == ME_RELEASE) snake_reset();
}

void app_snake_key(Window *w, int key)
{
    (void)w;
    if (key == 'r' || key == 'R') { snake_reset(); return; }
    if (key == ' ') { paused = !paused; return; }
    if (dead) { snake_reset(); return; }

    if ((key == 0xC8 || key == 'w' || key == 'W') && diry != 1) {
        dirx = 0; diry = -1; beep_click();
    } else if ((key == 0xD0 || key == 's' || key == 'S') && diry != -1) {
        dirx = 0; diry = 1; beep_click();
    } else if ((key == 0xCB || key == 'a' || key == 'A') && dirx != 1) {
        dirx = -1; diry = 0; beep_click();
    } else if ((key == 0xCD || key == 'd' || key == 'D') && dirx != -1) {
        dirx = 1; diry = 0; beep_click();
    }
}

static void step(void)
{
    if (dead || paused) return;

    int nx = snake[0].x + dirx;
    int ny = snake[0].y + diry;

    if (nx < 0 || nx >= SX || ny < 0 || ny >= SY) { dead = 1; beep_err(); return; }
    for (int i = 0; i < snk_len; i++)
        if (snake[i].x == nx && snake[i].y == ny) { dead = 1; beep_err(); return; }

    int eat = (nx == food.x && ny == food.y);

    for (int i = snk_len - 1; i > 0; i--) snake[i] = snake[i - 1];
    snake[0].x = nx; snake[0].y = ny;

    if (eat) { snk_len++; score += 10; beep_ok(); place_food(); }
}

void app_snake_draw(Window *w, Rect *c)
{
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(10, 14, 26));

    int cw = c->w - 16, ch = c->h - 46;
    int cell = cw / SX;
    if (ch / SY < cell) cell = ch / SY;
    if (cell < 1) cell = 1;

    int ox = c->x + 8, oy = c->y + 8;

    for (int i = 0; i <= SX; i++) vline(ox + i * cell, oy, SY * cell, rgb(20, 26, 44));
    for (int j = 0; j <= SY; j++) hline(ox, oy + j * cell, SX * cell, rgb(20, 26, 44));

    fill_rect(ox + food.x * cell + 1, oy + food.y * cell + 1, cell - 2, cell - 2, rgb(255, 90, 120));

    for (int i = 0; i < snk_len; i++) {
        u32 col = (i == 0) ? g_accent : mixc(g_accent, rgb(0, 0, 0), 60);
        fill_rect(ox + snake[i].x * cell + 1, oy + snake[i].y * cell + 1, cell - 2, cell - 2, col);
    }

    char s[32], n[12];
    strcpy(s, "Score: ");
    utoa_dec((u32)score, n);
    strcat(s, n);
    text(c->x + 8, c->y + c->h - 24, s, 1, rgb(220, 226, 240));

    if (dead) {
        const char *m = "GAME OVER - press R / click";
        text(c->x + c->w / 2 - text_w(m, 1) / 2, c->y + c->h / 2, m, 1, rgb(255, 120, 120));
    } else if (paused) {
        const char *m = "PAUSED (space)";
        text(c->x + c->w / 2 - text_w(m, 1) / 2, c->y + c->h / 2, m, 1, rgb(200, 220, 255));
    }

    if (g_ticks - last_move >= 7) { last_move = g_ticks; step(); }
}
