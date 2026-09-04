#include "icons.h"
#include "gfx.h"
#include "gui.h"

u32 g_accent_a = 0;
u32 g_accent_b = 0;

void icon_draw(int id, int x, int y, int s)
{
    u32 white = rgb(240, 244, 250);
    u32 dim = rgb(150, 158, 172);
    u32 dark = rgb(16, 18, 26);

    switch (id) {

    case ICON_KLOGO: {
        for (int j = 0; j < s; j++) {
            u32 c = mixc(g_accent_a, g_accent_b, (u8)((j * 255) / s));
            hline(x + 1, y + j + 1, s - 2, c);
        }
        blend_rect(x, y + s / 3, s, s / 3, rgb(255, 255, 255), 30);
        rect_outline(x, y, s + 1, s + 1, mixc(white, g_accent_a, 60));
        int sc = s >= 28 ? 2 : 1;
        text(x + s / 2 - 8 * sc / 2, y + s / 2 - 8 * sc / 2, "K", sc, white);
        break;
    }

    case ICON_TERM: {
        round_rect(x, y, s, s, s / 5, dark);
        rect_outline(x, y, s + 1, s + 1, rgb(70, 76, 92));
        int sc = s / 20 > 0 ? s / 20 : 1;
        if (sc > 3) sc = 3;
        text(x + s / 5, y + s / 4, ">", sc, g_accent_a);
        fill_rect(x + s / 5, y + s - s / 4 - sc * 2, s / 3, sc, white);
        break;
    }

    case ICON_FILES: {
        fill_rect(x + s / 7, y + s / 4, s / 2, s / 6, mixc(g_accent_a, dark, 130));
        round_rect(x + s / 12, y + s / 3, s * 10 / 12, s * 9 / 16, s / 8,
                   mixc(g_accent_a, white, 40));
        blend_rect(x + s / 12, y + s / 3, s * 10 / 12, s / 8,
                   rgb(255, 255, 255), 50);
        break;
    }

    case ICON_CALC: {
        round_rect(x, y, s, s, s / 5, dark);
        rect_outline(x, y, s + 1, s + 1, rgb(70, 76, 92));
        fill_rect(x + s / 6, y + s / 7, s * 2 / 3, s / 5,
                  mixc(g_accent_a, white, 30));
        for (int r = 0; r < 2; r++)
            for (int c = 0; c < 3; c++)
                circle_fill(x + s * (25 + c * 22) / 100,
                            y + s * (58 + r * 22) / 100,
                            s / 11, r == 0 ? dim : g_accent_b);
        break;
    }

    case ICON_DOODLE: {
        round_rect(x, y, s, s, s / 5, rgb(24, 27, 36));
        rect_outline(x, y, s + 1, s + 1, rgb(70, 76, 92));
        draw_line(x + s / 4, y + s * 3 / 4, x + s * 3 / 4 - 2, y + s / 4 + 2,
                  g_accent_a);
        circle_fill(x + s * 3 / 4, y + s / 4, s / 7, g_accent_b);
        draw_line(x + s * 3 / 4, y + s / 4 - s / 7, x + s / 4 + 2, y + s * 3 / 4 - 2,
                  mixc(g_accent_a, white, 90));
        break;
    }

    case ICON_SETTINGS: {
        u32 gc = mixc(dim, white, 35);
        int cx = x + s / 2, cy = y + s / 2;
        for (int a = 0; a < 360; a += 45)
            for (int rr = s / 3; rr <= s / 2; rr++) {
                int px, py;
                arc_point(cx, cy, rr, a, &px, &py);
                fill_rect(px - 1, py - 1, 3, 3, gc);
            }
        circle_ring(cx, cy, s / 3, gc);
        circle_ring(cx, cy, s / 3 + 1, gc);
        circle_fill(cx, cy, s / 6, g_accent_a);
        break;
    }

    case ICON_ABOUT: {
        int cx = x + s / 2, cy = y + s / 2;
        circle_fill(cx, cy, s / 2, mixc(g_accent_b, dark, 55));
        circle_ring(cx, cy, s / 2, g_accent_b);
        circle_ring(cx, cy, s / 2 - 1, g_accent_b);
        int sc = s / 20 ? s / 20 : 1;
        if (sc > 3) sc = 3;
        text(cx - 4 * sc, y + s / 6, "i", sc, white);
        break;
    }

    case ICON_POWER: {
        int cx = x + s / 2, cy = y + s / 2;
        arc(cx, cy, s / 3, -80, 260, white);
        vline(cx, y + s / 10, s / 4, white);
        break;
    }

    case ICON_REBOOT: {
        int cx = x + s / 2, cy = y + s / 2;
        arc(cx, cy, s / 3, 40, 320, white);
        int hx, hy;
        arc_point(cx, cy, s / 3, 40, &hx, &hy);
        draw_line(hx - 4, hy - 2, hx + 2, hy - 5, white);
        draw_line(hx - 4, hy - 2, hx + 1, hy + 3, white);
        break;
    }

    case ICON_FOLDER: {
        fill_rect(x + s / 7, y + s / 4, s / 2, s / 6, rgb(90, 96, 112));
        round_rect(x + s / 12, y + s / 3, s * 10 / 12, s * 9 / 16, s / 10,
                   rgb(120, 128, 148));
        break;
    }

    case ICON_TXT: {
        round_rect(x + s / 5, y, s * 3 / 5, s, s / 12, rgb(225, 228, 236));
        fill_rect(x + s / 5 + s * 3 / 5 - s / 5, y, s / 5, s / 5,
                  rgb(160, 166, 182));
        for (int l = 0; l < 4; l++)
            fill_rect(x + s * 3 / 10, y + s * (30 + l * 15) / 100, s * 2 / 5,
                      s / 14, rgb(110, 116, 132));
        break;
    }

    case ICON_IMG: {
        round_rect(x, y, s, s, s / 8, rgb(30, 34, 46));
        rect_outline(x, y, s + 1, s + 1, rgb(70, 76, 92));
        circle_fill(x + s * 3 / 4, y + s / 4, s / 8, rgb(255, 210, 110));
        draw_line(x + s / 6, y + s * 3 / 4, x + s * 2 / 5, y + s * 2 / 5, rgb(90, 200, 140));
        draw_line(x + s * 2 / 5, y + s * 2 / 5, x + s * 3 / 5, y + s * 3 / 4, rgb(90, 200, 140));
        draw_line(x + s / 2, y + s * 2 / 3, x + s * 3 / 4, y + s * 3 / 8, rgb(70, 170, 220));
        draw_line(x + s * 3 / 4, y + s * 3 / 8, x + s * 7 / 8, y + s * 2 / 3, rgb(70, 170, 220));
        break;
    }

    case ICON_SPEAKER: {
        for (int i = 0; i <= s / 3; i++)
            vline(x + i, y + s / 2 - i / 2 - 1, i + 2, white);
        fill_rect(x, y + s / 2 - 2, s / 3 + 1, 4, white);
        arc(x + s / 3 + 2, y + s / 2, s / 4, -55, 55, white);
        arc(x + s / 3 + 2, y + s / 2, s * 2 / 5, -45, 45, white);
        break;
    }

    case ICON_NET: {
        rect_outline(x + s / 6, y + s / 4, s * 2 / 3, s / 3, white);
        vline(x + s / 2, y + s * 7 / 12, s / 5, white);
        hline(x + s / 3, y + s * 7 / 8, s / 3, white);
        break;
    }

    case ICON_SHIELD: {
        int cx = x + s / 2;
        int top = y + s / 8;
        int bot = y + s * 7 / 8;
        int mid = y + s / 2;
        for (int j = top; j <= bot; j++) {
            int w_row;
            if (j < mid) {
                w_row = (s / 2 - 2) * (j - top) / (mid - top);
            } else {
                w_row = (s / 2 - 2) * (bot - j) / (bot - mid);
            }
            if (w_row < 1) w_row = 1;
            hline(cx - w_row, j, w_row * 2, g_accent_a);
        }
        for (int j = top; j <= bot; j++) {
            int w_row;
            if (j < mid) {
                w_row = (s / 2 - 4) * (j - top) / (mid - top);
            } else {
                w_row = (s / 2 - 4) * (bot - j) / (bot - mid);
            }
            if (w_row < 1) w_row = 1;
            putpx(cx - w_row, j, mixc(g_accent_a, white, 80));
            putpx(cx + w_row, j, mixc(g_accent_a, white, 80));
        }
if (s >= 20) {
            int sc = s / 8;
            if (sc < 2) sc = 2;
            draw_line(cx - sc, mid, cx - sc / 3, mid + sc * 2 / 3, white);
            draw_line(cx - sc / 3, mid + sc * 2 / 3, cx + sc, mid - sc, white);
        }
        break;
    }

    case ICON_IMAGE: {
        round_rect(x, y, s, s, s / 8, rgb(30, 34, 46));
        rect_outline(x, y, s + 1, s + 1, rgb(70, 76, 92));
        int cx = x + s / 2, cy = y + s / 2;
        circle_fill(cx + s / 4, cy - s / 4, s / 8, rgb(255, 210, 110));
        draw_line(cx - s / 6, cy + s / 4, cx - s / 20, cy + s / 10, rgb(90, 200, 140));
        draw_line(cx - s / 20, cy + s / 10, cx + s / 4, cy + s / 8, rgb(90, 200, 140));
        draw_line(cx + s / 8, cy - s / 8, cx + s / 2, cy + s / 4, rgb(70, 170, 220));
        draw_line(cx + s / 2, cy + s / 4, cx + s * 3 / 4, cy - s / 6, rgb(70, 170, 220));
        break;
    }

    case ICON_MUSIC: {
        int cx = x + s / 2, cy = y + s / 2;
        for (int i = 0; i <= s / 3; i++)
            vline(x + i, y + s / 2 - i / 2 - 1, i + 2, g_accent_a);
        fill_rect(x, y + s / 2 - 2, s / 3 + 1, 4, g_accent_a);
        arc(cx, cy, s / 3, -55, 55, g_accent_a);
        arc(cx, cy, s * 2 / 5, -45, 45, g_accent_a);
        break;
    }

    case ICON_SNAKE: {
        u32 gc = rgb(120, 230, 140);
        for (int k = 0; k < 4; k++) {
            int segx = x + s / 6 + k * s / 7;
            int segy = y + s / 2 + (k % 2 ? s / 8 : -s / 8);
            circle_fill(segx, segy, s / 9, mixc(gc, rgb(0, 0, 0), k * 10));
        }
        circle_fill(x + s * 5 / 6, y + s / 4, s / 8, g_accent);
        break;
    }

    case ICON_WIFI: {
        int cx = x + s / 2, cy = y + s * 3 / 4;
        arc(cx, cy, s / 3, -65, 65, white);
        arc(cx, cy, s / 2, -60, 60, white);
        arc(cx, cy, s * 2 / 3, -55, 55, white);
        circle_fill(cx, cy, s / 14, white);
        break;
    }

    case ICON_BATTERY: {
        int bw = s * 3 / 4, bh = s / 2;
        int bx = x + (s - bw) / 2, by = y + (s - bh) / 2;
        round_rect(bx, by, bw, bh, s / 12, rgb(40, 44, 54));
        rect_outline(bx, by, bw + 1, bh + 1, dim);
        fill_rect(bx + bw, by + bh / 3, s / 12, bh / 3, dim);
        round_rect(bx + 2, by + 2, (bw - 4) * 3 / 4, bh - 4, s / 16,
                   mixc(g_accent_a, white, 30));
        break;
    }
}
}
