#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "timer.h"
#include "mm.h"
#include "vfs.h"

extern int g_nightlight;
extern int g_auto_theme;
extern int g_focus_mode;
extern int g_mood;

static const struct {
    u32 a, b;
} accents[] = {
    { 0x35E0DA, 0xB46BFA },
    { 0xB46BFA, 0xFF5AA0 },
    { 0x6BE26B, 0x35E0DA },
    { 0xFFB050, 0xFF6A5A },
    { 0xFF5A8C, 0xB46BFA },
    { 0xC8D0E0, 0x8890A0 },
};
#define NACC 6

static const struct { int theme; const char *name; } walls[] = {
    { WP_AURORA, "aurora" },
    { WP_SUNSET, "sunset" },
    { WP_OCEAN, "ocean" },
    { WP_MONO, "mono" },
};

static const char *settings_categories[] = {
    "Personalization",
    "System",
    "Input",
    "Display",
    "Privacy",
    "About"
};
#define SETTINGS_CATS 6

static int g_settings_cat = 0;
static int g_settings_accent = 0;
static int g_settings_hover_accent = -1;
static int g_settings_hover_wall = -1;
static int g_settings_hover_btn = -1;

static void set_accent(int i)
{
    g_accent = accents[i].a;
    g_accent_a = accents[i].a;
    g_accent_b = accents[i].b;
    g_settings_accent = i;
    ui_request_redraw();
}

void app_settings_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w;
    if (ev != ME_PRESS) return;

    int nav_w = 160;
    if (lx < nav_w) {
        int cat_h = 44;
        int idx = ly / cat_h;
        if (idx >= 0 && idx < SETTINGS_CATS) {
            g_settings_cat = idx;
            ui_request_redraw();
            return;
        }
    }

    switch (g_settings_cat) {
    case 0: { // Personalization
        for (int i = 0; i < NACC; i++) {
            Rect sr = { 180 + i * 38, 44, 32, 32 };
            if (ui_in(sr, lx, ly)) { set_accent(i); return; }
        }
        for (int i = 0; i < 4; i++) {
            Rect wr = { 180 + i * 88, 108, 78, 48 };
            if (ui_in(wr, lx, ly)) { theme_set(walls[i].theme); return; }
        }
        break;
    }
    case 1: { // System
        // Clock toggle
        Rect ck = { 180, 44, 180, 36 };
        if (ui_in(ck, lx, ly)) { g_clock24 ^= 1; ui_request_redraw(); return; }
        break;
    }
    case 3: { // Display
        Rect nl = { 180, 110, 180, 36 };
        if (ui_in(nl, lx, ly)) { g_nightlight ^= 1; toast(g_nightlight ? "Night light on" : "Night light off"); ui_request_redraw(); return; }
        Rect at = { 180, 154, 180, 36 };
        if (ui_in(at, lx, ly)) { g_auto_theme ^= 1; toast(g_auto_theme ? "Adaptive theme on" : "Adaptive theme off"); ui_request_redraw(); return; }
        Rect fm = { 180, 198, 180, 36 };
        if (ui_in(fm, lx, ly)) { g_focus_mode ^= 1; toast(g_focus_mode ? "Focus mode on" : "Focus mode off"); ui_request_redraw(); return; }
        Rect md = { 180, 242, 180, 36 };
        if (ui_in(md, lx, ly)) { g_mood ^= 1; toast(g_mood ? "Mood on" : "Mood off"); ui_request_redraw(); return; }
        break;
    }
    case 5: { // About
        Rect ab = { 180, 180, 180, 36 };
        if (ui_in(ab, lx, ly)) win_open(APP_ABOUT);
        break;
    }
    }
}

static void set_cat_hover(int x, int y, int nav_w) {
    g_settings_hover_btn = -1;
    if (x < nav_w) {
        g_settings_hover_btn = y / 44;
    }
}

static void draw_category_nav(Rect *c) {
    int nav_w = 160;
    blend_rect(c->x, c->y, nav_w, c->h, rgb(14, 15, 22), 240);
    vline(c->x + nav_w - 1, c->y, c->h, rgb(38, 42, 54));

    for (int i = 0; i < SETTINGS_CATS; i++) {
        Rect ir = { c->x, c->y + i * 44, nav_w, 44 };
        int active = (i == g_settings_cat);
        int hover = ui_in(ir, ms_x, ms_y);

        u32 bg = active ? mixc(g_accent, rgb(18,20,28), 80) :
                 hover ? mixc(rgb(255,255,255), rgb(18,20,28), 30) : rgb(18,20,28);
        if (active) fill_rect(c->x + nav_w - 3, ir.y, 3, 44, g_accent);
        fill_rect(ir.x, ir.y, ir.w, ir.h, bg);

        text(c->x + 16, ir.y + 14, settings_categories[i], 1,
             active ? rgb(245,248,252) : (hover ? rgb(220,225,235) : rgb(160,168,184)));
    }
}

static void draw_personalization(Rect *c, int content_x, int content_y, int content_w) {
    text(content_x, content_y, "Accent Colors", 1, rgb(235, 238, 246));
    for (int i = 0; i < NACC; i++) {
        Rect sr = { content_x + i * 38, content_y + 24, 32, 32 };
        int hover = ui_in(sr, ms_x, ms_y);
        int active = (i == g_settings_accent);

        round_rect(sr.x, sr.y, sr.w, sr.h, 8, mixc(accents[i].a, accents[i].b, 128));
        rect_outline(sr.x, sr.y, sr.w + 1, sr.h + 1,
                     active ? rgb(255, 255, 255) : (hover ? rgb(180,188,202) : rgb(64, 70, 86)));
        if (active) rect_outline(sr.x - 2, sr.y - 2, sr.w + 5, sr.h + 5, g_accent);
        if (hover) blend_rect(sr.x, sr.y, sr.w, sr.h, rgb(255, 255, 255), 35);
    }

    text(content_x, content_y + 64, "Wallpapers", 1, rgb(235, 238, 246));
    for (int i = 0; i < 4; i++) {
        Rect wr = { content_x + i * 88, content_y + 88, 78, 48 };
        int active = (g_theme == walls[i].theme);
        int hover = ui_in(wr, ms_x, ms_y);

        u32 a, b;
        switch (walls[i].theme) {
        case WP_SUNSET: a = rgb(122, 30, 78); b = rgb(232, 115, 74); break;
        case WP_OCEAN:  a = rgb(10, 77, 110); b = rgb(18, 165, 165); break;
        case WP_MONO:   a = rgb(35, 38, 46); b = rgb(80, 85, 98); break;
        default:        a = rgb(27, 36, 71); b = rgb(58, 29, 94); break;
        }
        for (int j = 0; j < wr.h; j++)
            hline(wr.x, wr.y + j, wr.w, mixc(a, b, (u8)((j * 255) / wr.h)));

        if (active) rect_outline(wr.x - 2, wr.y - 2, wr.w + 5, wr.h + 5, g_accent);
        else rect_outline(wr.x, wr.y, wr.w + 1, wr.h + 1, rgb(64, 70, 86));

        if (hover) blend_rect(wr.x, wr.y, wr.w, wr.h, rgb(255, 255, 255), 25);
        text(wr.x + wr.w / 2 - text_w(walls[i].name, 1) / 2,
             wr.y + wr.h + 5, walls[i].name, 1, rgb(150, 156, 170));
    }
}

static void draw_system(Rect *c, int content_x, int content_y, int content_w) {
    text(content_x, content_y, "Date & Time", 1, rgb(235, 238, 246));

    Rect ck = { content_x, content_y + 24, 180, 36 };
    int hover = ui_in(ck, ms_x, ms_y);
    round_rect(ck.x, ck.y, ck.w, ck.h, 8,
               hover ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(ck.x, ck.y, ck.w + 1, ck.h + 1, hover ? g_accent : rgb(64, 70, 86));
    char lbl[24];
    strcpy(lbl, g_clock24 ? "24-hour format" : "12-hour format");
    text(ck.x + 12, ck.y + 11, lbl, 1, rgb(225, 229, 238));

    // Date format
    Rect df = { content_x, content_y + 70, 180, 36 };
    int hover2 = ui_in(df, ms_x, ms_y);
    round_rect(df.x, df.y, df.w, df.h, 8,
               hover2 ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(df.x, df.y, df.w + 1, df.h + 1, hover2 ? g_accent : rgb(64, 70, 86));
    text(df.x + 12, df.y + 11, "DD.MM.YYYY", 1, rgb(225, 229, 238));

    // Uptime
    u32 up = g_ticks / 100;
    char info[64];
    int y = content_y + 120;
    text(content_x, y, "System Uptime", 1, rgb(150, 156, 170)); y += 22;
    utoa_dec(up / 3600, info); strcat(info, "h "); utoa_dec((up/60)%60, info+strlen(info)); strcat(info, "m "); utoa_dec(up%60, info+strlen(info)); strcat(info, "s");
    text(content_x, y, info, 1, rgb(200, 208, 220)); y += 22;

    // Memory
    u32 used = heap_used() / 1024;
    u32 total = heap_total() / 1024;
    strcpy(info, "Memory: "); utoa_dec(used, info+8); strcat(info, " MB / "); utoa_dec(total, info+strlen(info)); strcat(info, " MB");
    text(content_x, y, info, 1, rgb(200, 208, 220)); y += 22;
    u32 used_pct = used * 100 / total;
    strcpy(info, "Usage: "); utoa_dec(used_pct, info+7); strcat(info, "%");
    u32 bar_w = (content_w - 20) * used_pct / 100;
    round_rect(content_x, y, content_w - 20, 8, 4, rgb(20, 22, 30));
    if (bar_w > 0) {
        u32 color = used_pct > 80 ? rgb(255,100,100) : used_pct > 50 ? rgb(255,200,80) : rgb(100,220,120);
        for (int j = 0; j < 6; j++)
            hline(content_x + 1, y + 1 + j, bar_w - 2, mixc(color, rgb(0,0,0), (u8)(j*40/6)));
    }
}

static void draw_input(Rect *c, int content_x, int content_y, int content_w) {
    text(content_x, content_y, "Keyboard", 1, rgb(235, 238, 246));
    int y = content_y + 24;

    // Repeat rate
    Rect rr = { content_x, y, 180, 36 };
    int hover = ui_in(rr, ms_x, ms_y);
    round_rect(rr.x, rr.y, rr.w, rr.h, 8,
               hover ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(rr.x, rr.y, rr.w + 1, rr.h + 1, hover ? g_accent : rgb(64, 70, 86));
    text(rr.x + 12, rr.y + 11, "Repeat rate: Normal", 1, rgb(225, 229, 238));
    y += 44;

    // Mouse
    Rect ms = { content_x, y, 180, 36 };
    int hover2 = ui_in(ms, ms_x, ms_y);
    round_rect(ms.x, ms.y, ms.w, ms.h, 8,
               hover2 ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(ms.x, ms.y, ms.w + 1, ms.h + 1, hover2 ? g_accent : rgb(64, 70, 86));
    text(ms.x + 12, ms.y + 11, "Mouse speed: 1.0x", 1, rgb(225, 229, 238));
}

static void draw_display(Rect *c, int content_x, int content_y, int content_w) {
    text(content_x, content_y, "Display", 1, rgb(235, 238, 246));
    int y = content_y + 24;

    // Resolution
    char res[32];
    utoa_dec(SW, res); strcat(res, " x "); utoa_dec(SH, res+strlen(res));
    text(content_x, y, res, 1, rgb(200, 208, 220)); y += 22;

    // Scale
    Rect sc = { content_x, y, 180, 36 };
    int hover = ui_in(sc, ms_x, ms_y);
    round_rect(sc.x, sc.y, sc.w, sc.h, 8,
               hover ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(sc.x, sc.y, sc.w + 1, sc.h + 1, hover ? g_accent : rgb(64, 70, 86));
    text(sc.x + 12, sc.y + 11, "Scale: 100%", 1, rgb(225, 229, 238));
    y += 44;

    // Night light
    Rect nl = { content_x, y, 180, 36 };
    int hover2 = ui_in(nl, ms_x, ms_y);
    round_rect(nl.x, nl.y, nl.w, nl.h, 8,
               hover2 ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(nl.x, nl.y, nl.w + 1, nl.h + 1, hover2 ? g_accent : rgb(64, 70, 86));
    text(nl.x + 12, nl.y + 11, g_nightlight ? "Night light: On" : "Night light: Off", 1, rgb(225, 229, 238));

    y += 44;
    Rect at = { content_x, y, 180, 36 };
    int ha = ui_in(at, ms_x, ms_y);
    round_rect(at.x, at.y, at.w, at.h, 8,
               ha ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(at.x, at.y, at.w + 1, at.h + 1, ha ? g_accent : rgb(64, 70, 86));
    text(at.x + 12, at.y + 11, g_auto_theme ? "Adaptive theme: On" : "Adaptive theme: Off", 1, rgb(225, 229, 238));

    y += 44;
    Rect fm = { content_x, y, 180, 36 };
    int hf = ui_in(fm, ms_x, ms_y);
    round_rect(fm.x, fm.y, fm.w, fm.h, 8,
               hf ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(fm.x, fm.y, fm.w + 1, fm.h + 1, hf ? g_accent : rgb(64, 70, 86));
    text(fm.x + 12, fm.y + 11, g_focus_mode ? "Focus mode: On" : "Focus mode: Off", 1, rgb(225, 229, 238));

    y += 44;
    Rect md = { content_x, y, 180, 36 };
    int hm = ui_in(md, ms_x, ms_y);
    round_rect(md.x, md.y, md.w, md.h, 8,
               hm ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(md.x, md.y, md.w + 1, md.h + 1, hm ? g_accent : rgb(64, 70, 86));
    text(md.x + 12, md.y + 11, g_mood ? "Mood: On" : "Mood: Off", 1, rgb(225, 229, 238));
}

static void draw_privacy(Rect *c, int content_x, int content_y, int content_w) {
    text(content_x, content_y, "Privacy & Security", 1, rgb(235, 238, 246));
    int y = content_y + 24;

    Rect pr = { content_x, y, 200, 36 };
    int hover = ui_in(pr, ms_x, ms_y);
    round_rect(pr.x, pr.y, pr.w, pr.h, 8,
               hover ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(pr.x, pr.y, pr.w + 1, pr.h + 1, hover ? g_accent : rgb(64, 70, 86));
    text(pr.x + 12, pr.y + 11, "Telemetry: Disabled", 1, rgb(225, 229, 238));
    y += 44;

    Rect fd = { content_x, y, 200, 36 };
    int hover2 = ui_in(fd, ms_x, ms_y);
    round_rect(fd.x, fd.y, fd.w, fd.h, 8,
               hover2 ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(fd.x, fd.y, fd.w + 1, fd.h + 1, hover2 ? g_accent : rgb(64, 70, 86));
    text(fd.x + 12, fd.y + 11, "File indexing: On", 1, rgb(225, 229, 238));
    y += 44;

    Rect se = { content_x, y, 200, 36 };
    int hover3 = ui_in(se, ms_x, ms_y);
    round_rect(se.x, se.y, se.w, se.h, 8,
               hover3 ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
    rect_outline(se.x, se.y, se.w + 1, se.h + 1, hover3 ? g_accent : rgb(64, 70, 86));
    text(se.x + 12, se.y + 11, "Auto updates: Check weekly", 1, rgb(225, 229, 238));
}

static void draw_about(Rect *c, int content_x, int content_y, int content_w) {
    text(content_x, content_y, "About KiKOS.11", 1, g_accent);
    int y = content_y + 30;

    text(content_x, y, "KiKOS.11 'Aurora'", 2, rgb(240, 244, 250)); y += 32;
    text(content_x, y, "Version 2026.08 Build", 1, rgb(180, 188, 202)); y += 22;
    text(content_x, y, "Custom i386 kernel with VBE graphics", 1, rgb(160, 168, 184)); y += 22;
    text(content_x, y, "Own bootloader, own kernel, own GUI", 1, rgb(160, 168, 184)); y += 22;
    text(content_x, y, "Own window manager (KiWM)", 1, rgb(160, 168, 184)); y += 30;

    // System specs
    char info[128];
    u32 used = heap_used() / 1024;
    u32 total = heap_total() / 1024;
    u32 up = g_ticks / 100;

    strcpy(info, "Memory: "); utoa_dec(used, info+8); strcat(info, " MB / "); utoa_dec(total, info+strlen(info)); strcat(info, " MB");
    text(content_x, y, info, 1, rgb(180, 188, 202)); y += 22;

    u32 uptime = g_ticks / 100;
    strcpy(info, "Uptime: "); utoa_dec(uptime / 3600, info+8); strcat(info, "h "); utoa_dec((uptime/60)%60, info+strlen(info)); strcat(info, "m "); utoa_dec(uptime%60, info+strlen(info)); strcat(info, "s");
    text(content_x, y, info, 1, rgb(180, 188, 202)); y += 22;

    // Button
    Rect ab = { content_x, y + 10, 180, 36 };
    int hover = ui_in(ab, ms_x, ms_y);
    round_rect(ab.x, ab.y, ab.w, ab.h, 8,
               hover ? mixc(g_accent, rgb(20,22,30), 90) : mixc(g_accent, rgb(20,22,30), 70));
    rect_outline(ab.x, ab.y, ab.w + 1, ab.h + 1, hover ? rgb(255,255,255) : g_accent);
    text(ab.x + ab.w/2 - text_w("View Details", 1)/2, ab.y + 11, "View Details", 1,
         hover ? rgb(245,248,252) : rgb(200,208,220));
}

void app_settings_draw(Window *w, Rect *c)
{
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(19, 21, 29));

    int nav_w = 160;
    int content_x = c->x + nav_w + 20;
    int content_y = c->y + 20;
    int content_w = c->w - nav_w - 40;

    // Category navigation
    draw_category_nav(c);

    // Content area
    switch (g_settings_cat) {
    case 0: draw_personalization(c, content_x, content_y, content_w); break;
    case 1: draw_system(c, content_x, content_y, content_w); break;
    case 2: draw_input(c, content_x, content_y, content_w); break;
    case 3: draw_display(c, content_x, content_y, content_w); break;
    case 4: draw_privacy(c, content_x, content_y, content_w); break;
    case 5: draw_about(c, content_x, content_y, content_w); break;
    }
}