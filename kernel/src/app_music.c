#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "timer.h"
#include "vfs.h"
#include <string.h>

#define MUSIC_MAX 16

static const char *g_music_exts[] = { ".ogg", ".mp3", ".wav", ".flac", 0 };

static VfsNode *g_music_files[MUSIC_MAX];
static int g_music_count = 0;
static int g_music_playing = -1;
static int g_music_paused = 0;
static int g_music_pos = 0;
static int g_music_duration = 180;
static int g_music_volume = 70;
static int g_music_repeat = 0;
static int g_music_shuffle = 0;

static int music_is_audio(const char *name) {
    for (int i = 0; g_music_exts[i]; i++) {
        const char *ext = strrchr(name, '.');
        if (ext && strcmp(ext, g_music_exts[i]) == 0) return 1;
    }
    return 0;
}

static void music_scan_dir(VfsNode *dir) {
    g_music_count = 0;
    if (!dir || dir->type != VFS_DIR) return;
    for (int i = 0; i < dir->child_count; i++) {
        VfsNode *n = dir->children[i];
        if (n->type == VFS_FILE && music_is_audio(n->name)) {
            g_music_files[g_music_count++] = n;
            if (g_music_count >= MUSIC_MAX) break;
        }
    }
}

static void music_format_time(int sec, char *buf) {
    utoa_dec(sec / 60, buf);
    strcat(buf, ":");
    if ((sec % 60) < 10) strcat(buf, "0");
    utoa_dec(sec % 60, buf + strlen(buf));
}

void app_music_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    if (ev != ME_PRESS) return;

    Rect c = win_content(w);
    int w_w = c.w;

    // Playlist area
    if (ly > 40 && ly < c.h - 100) {
        int idx = (ly - 40) / 36;
        if (idx >= 0 && idx < g_music_count) {
            g_music_playing = idx;
            g_music_paused = 0;
            g_music_pos = 0;
            ui_request_redraw();
        }
        return;
    }

    // Controls
    int ctrl_y = c.h - 90;
    if (ly >= ctrl_y && ly < c.h - 20) {
        int btn_w = 44, btn_h = 44;
        int start_x = (w_w - btn_w * 5) / 2;
        int btn_y = ctrl_y + 10;

        // Shuffle
        if (lx >= start_x && lx < start_x + btn_w) { g_music_shuffle ^= 1; ui_request_redraw(); }
        start_x += btn_w + 10;
        // Prev
        if (lx >= start_x && lx < start_x + btn_w) { if (g_music_playing > 0) g_music_playing--; ui_request_redraw(); }
        start_x += btn_w + 10;
        // Play/Pause
        if (lx >= start_x && lx < start_x + btn_w) { g_music_paused ^= 1; ui_request_redraw(); }
        start_x += btn_w + 10;
        // Next
        if (lx >= start_x && lx < start_x + btn_w) { if (g_music_playing < g_music_count - 1) g_music_playing++; ui_request_redraw(); }
        start_x += btn_w + 10;
        // Repeat
        if (lx >= start_x && lx < start_x + btn_w) { g_music_repeat ^= 1; ui_request_redraw(); }

        // Progress bar click
        if (ly >= c.h - 30 && ly < c.h - 10) {
            g_music_pos = g_music_duration * (lx - 20) / (w_w - 40);
            if (g_music_pos < 0) g_music_pos = 0;
            if (g_music_pos > g_music_duration) g_music_pos = g_music_duration;
            ui_request_redraw();
        }
    }

    // Volume
    if (ly >= c.h - 20 && ly < c.h) {
        int vol_x = (w_w - 100) / 2;
        if (lx >= vol_x && lx < vol_x + 100) {
            g_music_volume = (lx - vol_x) * 100 / 100;
            if (g_music_volume < 0) g_music_volume = 0;
            if (g_music_volume > 100) g_music_volume = 100;
            ui_request_redraw();
        }
    }
}

void app_music_key(Window *w, int key) {
    (void)w;
    switch (key) {
    case ' ': { g_music_paused ^= 1; ui_request_redraw(); break; }
    case 0x4B: { if (g_music_playing > 0) g_music_playing--; ui_request_redraw(); break; }
    case 0x4D: { if (g_music_playing < g_music_count - 1) g_music_playing++; ui_request_redraw(); break; }
    case 0x48: { g_music_volume += 5; if (g_music_volume > 100) g_music_volume = 100; ui_request_redraw(); break; }
    case 0x50: { g_music_volume -= 5; if (g_music_volume < 0) g_music_volume = 0; ui_request_redraw(); break; }
    }
}

static void draw_waveform(Rect *area, int pos, int dur, int color) {
    if (dur <= 0) return;
    int w = area->w;
    int h = area->h;
    for (int i = 0; i < w; i += 2) {
        int sample = (i * 17 + pos * 13 + (int)g_ticks * 7) % (h - 4);
        int bar_h = 4 + (sample % (h - 8));
        int y = area->y + h - bar_h;
        u32 c = (i * 100 / w < pos * 100 / dur) ? (u32)color : rgb(80, 88, 104);
        vline(area->x + i, y, bar_h, c);
        vline(area->x + i + 1, y, bar_h, c);
    }
}

void app_music_draw(Window *w, Rect *c) {
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(18, 20, 28));

    // Sidebar - Playlist
    int sidebar_w = 220;
    blend_rect(c->x, c->y, sidebar_w, c->h, rgb(14, 15, 22), 240);
    vline(c->x + sidebar_w - 1, c->y, c->h, rgb(38, 42, 54));

    text(c->x + 14, c->y + 14, "Library", 1, rgb(235, 238, 246));

    if (g_music_count == 0) {
        text(c->x + 14, c->y + 44, "No music in Music folder", 1, rgb(100, 106, 120));
    } else {
        for (int i = 0; i < g_music_count; i++) {
            int item_y = c->y + 40 + i * 36;
            if (item_y + 36 > c->y + c->h) break;
            VfsNode *n = g_music_files[i];
            int playing = (i == g_music_playing && !g_music_paused);
            int hover = ui_in((Rect){c->x + 4, item_y, sidebar_w - 8, 34}, ms_x, ms_y);

            u32 bg = playing ? mixc(g_accent, rgb(18,20,28), 70) :
                     hover ? mixc(rgb(255,255,255), rgb(18,20,28), 30) : (i % 2 ? rgb(20,22,30) : rgb(18,20,28));
            fill_rect(c->x + 4, item_y, sidebar_w - 8, 34, bg);

            icon_draw(ICON_SPEAKER, c->x + 10, item_y + 8, 18);
            text(c->x + 34, item_y + 10, n->name, 1,
                 playing ? rgb(245,248,252) : (hover ? rgb(220,225,235) : rgb(180,188,202)));

            if (playing) {
                int eq_x = c->x + sidebar_w - 30;
                int h1 = 4 + (rand32() % 14);
                int h2 = 4 + (rand32() % 14);
                int h3 = 4 + (rand32() % 14);
                fill_rect(eq_x, item_y + 17 - h1/2, 4, h1, g_accent);
                fill_rect(eq_x + 6, item_y + 17 - h2/2, 4, h2, g_accent);
                fill_rect(eq_x + 12, item_y + 17 - h3/2, 4, h3, g_accent);
            }
        }
    }

    // Main area
    int main_x = c->x + sidebar_w;
    int main_w = c->w - sidebar_w;

    // Now playing
    int np_y = c->y + 20;
    if (g_music_playing >= 0 && g_music_playing < g_music_count) {
        VfsNode *n = g_music_files[g_music_playing];
        text(main_x + 20, np_y, "Now Playing", 1, rgb(150, 156, 170)); np_y += 18;
        text(main_x + 20, np_y, n->name, 2, rgb(240, 244, 250)); np_y += 28;
        text(main_x + 20, np_y, "Unknown Artist", 1, rgb(140, 148, 164)); np_y += 40;

        // Album art placeholder
        Rect art = { main_x + 20, np_y, 120, 120 };
        round_rect(art.x, art.y, art.w, art.h, 12, rgb(22, 24, 34));
        rect_outline(art.x, art.y, art.w + 1, art.h + 1, rgb(52, 58, 74));
        circle_fill(art.x + art.w/2, art.y + art.h/2, 30, g_accent);
        np_y += 140;

        // Progress bar
        Rect prog = { main_x + 20, np_y, main_w - 40, 6 };
        round_rect(prog.x, prog.y, prog.w, prog.h, 3, rgb(40, 44, 58));
        if (g_music_duration > 0) {
            int filled = prog.w * g_music_pos / g_music_duration;
            if (filled > 0) round_rect(prog.x, prog.y, filled, prog.h, 3, g_accent);
        }
        np_y += 14;

        // Time
        char time_buf[16];
        music_format_time(g_music_pos, time_buf);
        strcat(time_buf, " / ");
        music_format_time(g_music_duration, time_buf + strlen(time_buf));
        text(prog.x + prog.w/2 - text_w(time_buf, 1)/2, np_y, time_buf, 1, rgb(140, 148, 164));
        np_y += 20;
    } else {
        text(main_x + 20, np_y, "Select a track to play", 1, rgb(100, 106, 120));
    }

    // Waveform
    Rect wave = { main_x + 20, c->h - 160, main_w - 40, 80 };
    if (g_music_playing >= 0) draw_waveform(&wave, g_music_pos, g_music_duration, g_accent);

    // Controls
    int ctrl_y = c->h - 90;
    int btn_w = 44, btn_h = 44;
    int start_x = (main_w - btn_w * 5) / 2;
    int btn_y = ctrl_y + 10;

    const char *ctrl_labels[] = { "Shuffle", "Prev", "Play", "Next", "Repeat" };
    u32 ctrl_colors[] = {
        g_music_shuffle ? g_accent : rgb(100,106,120),
        rgb(200,208,220),
        g_music_paused ? rgb(200,208,220) : g_accent,
        rgb(200,208,220),
        g_music_repeat ? g_accent : rgb(100,106,120)
    };

    for (int i = 0; i < 5; i++) {
        int bx = start_x + i * (btn_w + 10);
        Rect btn = { bx, btn_y, btn_w, btn_h };
        int hover = ui_in(btn, ms_x, ms_y);

        round_rect(btn.x, btn.y, btn.w, btn.h, 8,
                   hover ? mixc(g_accent, rgb(20,22,30), 90) : rgb(22, 24, 34));
        rect_outline(btn.x, btn.y, btn.w + 1, btn.h + 1,
                     hover ? g_accent : (i == 2 && !g_music_paused ? g_accent : rgb(52, 58, 74)));

        if (i == 2) { // Play/Pause
            if (!g_music_paused) {
                fill_rect(bx + 12, btn_y + 10, 5, 24, ctrl_colors[i]);
                fill_rect(bx + 22, btn_y + 10, 5, 24, ctrl_colors[i]);
            } else {
                draw_line(bx + 13, btn_y + 10, bx + 13, btn_y + 34, ctrl_colors[i]);
                draw_line(bx + 13, btn_y + 10, bx + 30, btn_y + 22, ctrl_colors[i]);
                draw_line(bx + 13, btn_y + 34, bx + 30, btn_y + 22, ctrl_colors[i]);
            }
        } else if (i == 1) { // Prev
            draw_line(bx + 25, btn_y + 10, bx + 15, btn_y + 22, ctrl_colors[i]);
            draw_line(bx + 15, btn_y + 22, bx + 25, btn_y + 34, ctrl_colors[i]);
            vline(bx + 13, btn_y + 10, 24, ctrl_colors[i]);
        } else if (i == 3) { // Next
            draw_line(bx + 15, btn_y + 10, bx + 25, btn_y + 22, ctrl_colors[i]);
            draw_line(bx + 25, btn_y + 22, bx + 15, btn_y + 34, ctrl_colors[i]);
            vline(bx + 29, btn_y + 10, 24, ctrl_colors[i]);
        } else {
            text(bx + btn_w/2 - text_w(ctrl_labels[i], 1)/2, btn_y + 14, ctrl_labels[i], 1, ctrl_colors[i]);
        }
    }

    // Progress bar
    Rect prog = { main_x + 20, c->h - 30, main_w - 40, 6 };
    round_rect(prog.x, prog.y, prog.w, prog.h, 3, rgb(40, 44, 58));
    int filled = 0;
    if (g_music_duration > 0) {
        filled = prog.w * g_music_pos / g_music_duration;
        if (filled > 0) round_rect(prog.x, prog.y, filled, prog.h, 3, g_accent);
    }
    circle_fill(prog.x + filled, prog.y + 3, 6, g_accent);

    // Volume
    int vol_y = c->h - 16;
    int vol_x = (main_w - 100) / 2;
    text(main_x + vol_x - 60, vol_y, "Vol", 1, rgb(140,148,164));
    round_rect(main_x + vol_x, vol_y - 2, 100, 4, 2, rgb(40, 44, 58));
    round_rect(main_x + vol_x, vol_y - 2, g_music_volume, 4, 2, g_accent);
    circle_fill(main_x + vol_x + g_music_volume, vol_y, 6, g_accent);
}