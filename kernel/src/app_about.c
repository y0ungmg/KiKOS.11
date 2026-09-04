#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "timer.h"

void app_about_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w; (void)lx; (void)ly; (void)ev;
}

void app_about_draw(Window *w, Rect *c)
{
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(17, 19, 27));

    int cx = c->x + c->w / 2;
    icon_draw(ICON_KLOGO, cx - 32, c->y + 18, 64);

    const char *name = "KiKOS.11";
    text(cx - text_w(name, 3) / 2, c->y + 92, name, 3, rgb(240, 244, 250));

    const char *sub = "'Aurora' - build 2026.08";
    text(cx - text_w(sub, 1) / 2, c->y + 120, sub, 1, g_accent);

    blend_rect(c->x + 30, c->y + 138, c->w - 60, 1, rgb(90, 96, 116), 160);

    char line[80], n[16];
    int y = c->y + 150;

    strcpy(line, "Resolution: ");
    utoa_dec((u32)SW, n); strcat(line, n); strcat(line, " x ");
    utoa_dec((u32)SH, n); strcat(line, n);
    text(c->x + 34, y, line, 1, rgb(200, 206, 220)); y += 14;

    strcpy(line, "CPU: ");
    {
        const char *br = cpu_brand();
        strncat(line, br, 38);
    }
    text(c->x + 34, y, line, 1, rgb(200, 206, 220)); y += 14;

    strcpy(line, "Memory: ~");
    utoa_dec(total_mem_kb() / 1024, n);
    strcat(line, n);
    strcat(line, " MB");
    text(c->x + 34, y, line, 1, rgb(200, 206, 220)); y += 14;

    u32 up = g_ticks / 100;
    strcpy(line, "Uptime: ");
    utoa_dec(up / 3600, n); strcat(line, n); strcat(line, "h ");
    utoa_dec((up / 60) % 60, n); strcat(line, n); strcat(line, "m ");
    utoa_dec(up % 60, n); strcat(line, n); strcat(line, "s");
    text(c->x + 34, y, line, 1, rgb(200, 206, 220)); y += 20;

    const char *foot = "own bootloader | own kernel | own GUI";
    text(cx - text_w(foot, 1) / 2, y, foot, 1, rgb(130, 137, 152)); y += 16;

    const char *love = "crafted for y0ungmg";
    text(cx - text_w(love, 1) / 2, y, love, 1,
         mixc(g_accent_a, rgb(255, 255, 255), 40));
}
