#include "gui.h"
#include "gfx.h"
#include "lib.h"
#include "icons.h"
#include "timer.h"
#include "rtc.h"
#include "ps2mouse.h"
#include "ps2kbd.h"
#include "apps.h"
#include "power.h"
#include "com1.h"
#include "pcspk.h"
#include "mm.h"

u32 g_accent = 0;
int g_theme = WP_AURORA;
int g_clock24 = 1;

volatile u32 g_input_epoch = 0;

Window g_wins[16];
int g_nwins = 0;
Window *g_focus = 0;
int g_gui_active = 0;

/* ----- idle screensaver (starfield) ----- */
#define N_STARS 170
typedef struct { int x, y, z; } Star;
static Star   stars[N_STARS];
static int    stars_inited = 0;
static u32    last_input_tick = 0;
static int    ss_on = 0;
#define SS_IDLE_TICKS 2000   /* ~20s at 100Hz */

static int prev_l = 0, prev_r = 0;
static int press_l_x, press_l_y;
static int l_pressed_now, l_released_now, r_pressed_now;
static Window *drag_win = 0;
static Window *resize_win = 0;
static int drag_offx, drag_offy;
static int resize_offx, resize_offy;

static int start_open = 0;
static Rect start_btn_r;

static int ctx_open = 0;
static Rect ctx_r;
enum { CTX_TERM, CTX_WALL, CTX_REFRESH, CTX_ABOUT, CTX_PAL, CTX_GLANCE, CTX_NIGHT, CTX_FOCUS, CTX_MOOD, CTX_N };

static int desk_sel = -1;
static int last_click_tick = 0;
static int last_click_icon = -1;

static const struct { int icon; const char *label; } desk_icons[] = {
    { ICON_TERM, "Terminal" },
    { ICON_FILES, "Files" },
    { ICON_DOODLE, "Doodle" },
    { ICON_SHIELD, "Antivirus" },
    { ICON_TXT, "Text Editor" },
    { ICON_SETTINGS, "System Monitor" },
    { ICON_IMAGE, "Image Viewer" },
    { ICON_MUSIC, "Music Player" },
    { ICON_ABOUT, "About" },
    { ICON_SNAKE, "Snake" },
    { ICON_DOODLE, "Game of Life" },
    { ICON_CALC, "Slide Puzzle" },
};
#define N_DESK_ICONS 12

static const int pins[] = { APP_TERM, APP_FILES, APP_CALC, APP_DOODLE, APP_AV, APP_EDIT, APP_SYSMON, APP_IMGVIEW, APP_MUSIC, APP_SETTINGS, APP_SNAKE, APP_KALEIDOSCOPE, APP_GOL, APP_SLIDE };
#define N_PINS 14

/* ----- night light, aero-snap, quick settings, start search ----- */
int g_nightlight = 0;
int g_auto_theme = 0;
int g_focus_mode = 0;
int g_mood = 0;

static Rect snap_r;
static int snap_on = 0;

static int qs_open = 0;
static int qs_wifi = 1, qs_bt = 1, qs_air = 0;
static int qs_vol = 70, qs_bright = 100;

static const struct { int app; const char *label; } start_items[] = {
    { APP_TERM, "Terminal" },
    { APP_FILES, "Files" },
    { APP_CALC, "Calculator" },
    { APP_DOODLE, "Doodle" },
    { APP_AV, "Antivirus" },
    { APP_EDIT, "Text Editor" },
    { APP_SYSMON, "System Monitor" },
    { APP_IMGVIEW, "Image Viewer" },
    { APP_MUSIC, "Music Player" },
    { APP_SETTINGS, "Settings" },
    { APP_ABOUT, "About KiKOS" },
    { APP_SNAKE, "Snake" },
    { APP_KALEIDOSCOPE, "Kaleidoscope" },
    { APP_GOL, "Game of Life" },
    { APP_SLIDE, "Slide Puzzle" },
};
#define N_START_ITEMS 15

static char start_search[32];
static int start_search_len = 0;

static int palette_open = 0;
static char pal_buf[48];
static int pal_len = 0;
static int trail_x[24], trail_y[24], trail_n = 0;

static int glance_open = 0;
static int glance_x = 0;
static Rect glance_btn_r;

#define N_TOAST 4
static struct { char msg[64]; u32 born; int on; } toasts[N_TOAST];

static void bump(void) { g_input_epoch++; }

void ui_request_redraw(void)
{
    g_input_epoch++;
}

u32 ui_hover_bg(u32 base, Rect r)
{
    return ui_in(r, ms_x, ms_y) ? mixc(base, rgb(255, 255, 255), 26) : base;
}

int ui_in(Rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void ui_panel(int x, int y, int w, int h, int rad, u32 color, u8 alpha)
{
    round_rect_blend(x, y, w, h, rad, color, alpha);
}

static int clicked(Rect r)
{
    return l_released_now && ui_in(r, ms_x, ms_y) && ui_in(r, press_l_x, press_l_y);
}

int ui_button(Rect r, const char *label)
{
    u32 bg = ui_hover_bg(rgb(38, 42, 54), r);
    if (ms_btn_l && ui_in(r, ms_x, ms_y)) bg = mixc(bg, rgb(0, 0, 0), 40);
    round_rect(r.x, r.y, r.w, r.h, 8, bg);
    rect_outline(r.x, r.y, r.w + 1, r.h + 1, rgb(70, 76, 92));
    if (label)
        text(r.x + r.w / 2 - text_w(label, 1) / 2,
             r.y + r.h / 2 - 4, label, 1, rgb(235, 238, 245));
    return clicked(r);
}

Rect win_content(Window *w)
{
    Rect c = { w->r.x + 1, w->r.y + TITLE_H + 1, w->r.w - 2, w->r.h - TITLE_H - 2 };
    return c;
}

Window *win_by_app(int app)
{
    for (int i = 0; i < g_nwins; i++)
        if (g_wins[i].used && g_wins[i].app == app)
            return &g_wins[i];
    return 0;
}

static Window *win_alloc(int app, const char *title, int ww, int wh)
{
    if (g_nwins >= 16) return 0;
    Window *w = &g_wins[g_nwins++];
    memset(w, 0, sizeof(*w));
    w->used = 1;
    w->app = app;
    strncpy(w->title, title, sizeof(w->title) - 1);
    static int cascade = 0;
    w->r.w = ww;
    w->r.h = wh;
    w->r.x = 130 + (cascade % 6) * 34;
    w->r.y = 80 + (cascade % 6) * 30;
    cascade++;
    if (w->r.x + ww > SW - 20) w->r.x = SW - ww - 20;
    if (w->r.y + wh > SH - TASKBAR_H - 10) w->r.y = 40;
    return w;
}

static void win_compaction_done(void)
{
    g_focus = 0;
    for (int i = g_nwins - 1; i >= 0; i--) {
        if (g_wins[i].visible) { g_focus = &g_wins[i]; break; }
    }
}

static void win_close(Window *w)
{
    if (!w) return;
    if (w->anim == 0 || w->anim == 2) {
        /* start closing animation */
        w->anim = 2;
        w->anim_tick = g_ticks;
    }
}

static void win_close_finish(Window *w)
{
    if (!w) return;
    w->visible = 0;
    w->maximized = 0;
    w->anim = 0;
    int idx = (int)(w - g_wins);
    if (idx >= 0 && idx < g_nwins) {
        for (int i = idx; i < g_nwins - 1; i++) g_wins[i] = g_wins[i + 1];
        g_nwins--;
        memset(&g_wins[g_nwins], 0, sizeof(Window));
    }
    win_compaction_done();
}

void focus_win(Window *w)
{
    if (!w) return;
    int idx = (int)(w - g_wins);
    if (idx < 0 || idx >= g_nwins) return;
    Window tmp = g_wins[idx];
    for (int i = idx; i < g_nwins - 1; i++) g_wins[i] = g_wins[i + 1];
    g_wins[g_nwins - 1] = tmp;
    g_focus = &g_wins[g_nwins - 1];
}

Window *win_open(int app)
{
    Window *w = win_by_app(app);
    if (!w) {
        switch (app) {
        case APP_TERM:     w = win_alloc(app, "Terminal", 560, 380); app_term_open(w); break;
        case APP_FILES:    w = win_alloc(app, "Files", 560, 380); break;
        case APP_CALC:     w = win_alloc(app, "Calculator", 264, 372); break;
        case APP_DOODLE:   w = win_alloc(app, "Doodle", 560, 430); app_doodle_init(); break;
        case APP_SETTINGS: w = win_alloc(app, "Settings", 460, 390); break;
        case APP_ABOUT:    w = win_alloc(app, "About KiKOS", 400, 320); break;
        case APP_AV:       w = win_alloc(app, "KiKOS Antivirus", 500, 440); break;
        case APP_EDIT:     w = win_alloc(app, "Text Editor", 600, 450); break;
        case APP_SYSMON:   w = win_alloc(app, "System Monitor", 580, 480); break;
        case APP_IMGVIEW:  w = win_alloc(app, "Image Viewer", 640, 480); break;
        case APP_MUSIC:    w = win_alloc(app, "Music Player", 600, 500); break;
        case APP_SNAKE:    w = win_alloc(app, "Snake", 480, 400); app_snake_open(w); break;
        case APP_KALEIDOSCOPE: w = win_alloc(app, "Kaleidoscope", 480, 400); app_kaleido_open(w); break;
        case APP_GOL:    w = win_alloc(app, "Game of Life", 520, 400); app_gol_open(w); break;
        case APP_SLIDE: w = win_alloc(app, "Slide Puzzle", 440, 440); app_slide_open(w); break;
        }
        if (!w) return 0;
    }
    beep_click();
    w->visible = 1;
    w->open_tick = g_ticks;
    w->anim_tick = g_ticks;
    w->anim = 1;
    focus_win(w);
    bump();
    return w;
}

void theme_apply(void)
{
    switch (g_theme) {
    case WP_SUNSET: g_accent = rgb(255, 140, 90); g_accent_a = rgb(255, 150, 70); g_accent_b = rgb(240, 70, 120); break;
    case WP_OCEAN:  g_accent = rgb(60, 220, 200); g_accent_a = rgb(60, 210, 230); g_accent_b = rgb(40, 110, 240); break;
    case WP_MONO:   g_accent = rgb(190, 196, 208); g_accent_a = rgb(150, 155, 165); g_accent_b = rgb(210, 214, 224); break;
    default:        g_accent = rgb(53, 224, 218); g_accent_a = rgb(53, 224, 218); g_accent_b = rgb(180, 100, 250); break;
    }
    wall_render(g_theme);
}

void theme_set(int t)
{
    if (t != g_theme) {
        g_theme = t;
        theme_apply();
    }
    bump();
}

/* ---------------- rendering ---------------- */

static void draw_desktop_icons(void)
{
    for (int i = 0; i < N_DESK_ICONS; i++) {
        int ix = 28, iy = 24 + i * 92;
        if (desk_sel == i) {
            round_rect_blend(ix - 8, iy - 8, 84, 84, 12, rgb(255, 255, 255), 28);
            rect_outline(ix - 8, iy - 8, 85, 85, mixc(rgb(255, 255, 255), g_accent, 120));
        } else if (ui_in((Rect){ ix - 8, iy - 8, 84, 84 }, ms_x, ms_y)) {
            round_rect_blend(ix - 8, iy - 8, 84, 84, 12, rgb(255, 255, 255), 14);
        }
        icon_draw(desk_icons[i].icon, ix + 20, iy, 44);
        const char *lb = desk_icons[i].label;
        text_blend(ix + 36 - text_w(lb, 1) / 2, iy + 50, lb, 1, rgb(255, 255, 255), 210);
    }
}

static void draw_window_chrome(Window *w)
{
    Rect r = w->r;
    int focused = (w == g_focus);

    round_rect_blend(r.x + 6, r.y + 10, r.w, r.h, 13, rgb(0, 0, 0), focused ? 78 : 58);
    round_rect_blend(r.x + 3, r.y + 5, r.w, r.h, 12, rgb(0, 0, 0), focused ? 55 : 42);
    round_rect_blend(r.x + 1, r.y + 2, r.w, r.h, 11, rgb(0, 0, 0), focused ? 42 : 30);
    round_rect(r.x, r.y, r.w, r.h, 10, rgb(20, 22, 30));

    if (focused) {
        int glow_h = r.h / 4;
        blend_rect(r.x + 2, r.y + 1, r.w - 4, glow_h,
                   mixc(g_accent, rgb(255, 255, 255), 120), 10);
    }

    for (int j = 0; j < TITLE_H; j++)
        blend_rect(r.x + 2, r.y + j, r.w - 4, 1,
                   mixc(rgb(44, 48, 66), g_accent, 60), (u8)(40 - 30 * j / TITLE_H));

    for (int j = 0; j < r.h; j++) {
        int li = 0;
        gfx_row_inset(j, r.h, 10, &li);
        u32 lc = focused ? g_accent : rgb(62, 68, 84);
        u32 rc = focused ? mixc(g_accent, rgb(0,0,0), 60) : rgb(52, 58, 72);
        putpx(r.x + li, r.y + j, lc);
        putpx(r.x + r.w - 1 - li, r.y + j, rc);
        if (focused) {
            putpx(r.x + li + 1, r.y + j, mixc(lc, rgb(0,0,0), 80));
            putpx(r.x + r.w - 2 - li, r.y + j, mixc(rc, rgb(0,0,0), 80));
        }
    }
    hline(r.x + 10, r.y, r.w - 20, focused ? g_accent : rgb(62, 68, 84));
    hline(r.x + 10, r.y + r.h - 1, r.w - 20, rgb(62, 68, 84));
    fill_rect(r.x + 1, r.y + TITLE_H, r.w - 2, focused ? 2 : 1, focused ? g_accent : rgb(45, 49, 62));
    {
        u8 pa = (u8)(50 + 45 * ((g_ticks % 150) * 255 / 150) / 255);
        blend_rect(r.x + 12, r.y + TITLE_H, r.w - 24, 1, g_accent, pa);
    }

    text(r.x + 12, r.y + TITLE_H / 2 - 4, w->title, 1, focused ? rgb(245, 247, 252) : rgb(150, 156, 170));
    icon_draw(w->app == APP_TERM ? ICON_TERM : w->app == APP_FILES ? ICON_FILES :
              w->app == APP_CALC ? ICON_CALC : w->app == APP_DOODLE ? ICON_DOODLE :
              w->app == APP_SETTINGS ? ICON_SETTINGS : w->app == APP_AV ? ICON_SHIELD :
               w->app == APP_EDIT ? ICON_TXT : w->app == APP_SYSMON ? ICON_SETTINGS :
               w->app == APP_IMGVIEW ? ICON_IMAGE : w->app == APP_MUSIC ? ICON_MUSIC :
               w->app == APP_SNAKE ? ICON_SNAKE : w->app == APP_KALEIDOSCOPE ? ICON_DOODLE : w->app == APP_GOL ? ICON_DOODLE : ICON_ABOUT,
              r.x + r.w / 2 - 9, r.y + 6, 18);

    int bw = 26;
    int by = r.y + 6;
    Rect bc = { r.x + r.w - bw - 8, by, bw, bw };
    Rect bm = { r.x + r.w - 2 * bw - 12, by, bw, bw };
    Rect bn = { r.x + r.w - 3 * bw - 16, by, bw, bw };

    int hov_c = ui_in(bc, ms_x, ms_y);
    int hov_m = ui_in(bm, ms_x, ms_y);
    int hov_n = ui_in(bn, ms_x, ms_y);

    circle_fill(bc.x + bc.w / 2, bc.y + bc.h / 2, bc.w / 2 - 1,
                hov_c ? rgb(232, 84, 92) : mixc(g_accent, rgb(18, 20, 28), 110));
    round_rect(bm.x, bm.y, bm.w, bm.h, 6, hov_m ? rgb(68, 74, 92) : rgb(40, 43, 54));
    round_rect(bn.x, bn.y, bn.w, bn.h, 6, hov_n ? rgb(68, 74, 92) : rgb(40, 43, 54));

    text(bc.x + bc.w / 2 - 4, bc.y + bc.h / 2 - 4, "K", 1,
         hov_c ? rgb(255, 255, 255) : rgb(232, 236, 246));
    hline(bm.x + 8, bm.y + bw / 2, bw - 16, rgb(214, 218, 228));
    if (w->maximized) {
        rect_outline(bn.x + 8, bn.y + 8, bw - 16, bw - 16, rgb(214, 218, 228));
        rect_outline(bn.x + 10, bn.y + 10, bw - 20, bw - 20, rgb(40, 43, 54));
    } else {
        rect_outline(bn.x + 8, bn.y + 8, bw - 16, bw - 16, rgb(214, 218, 228));
    }

    // Resize handle (bottom-right)
    Rect br = { r.x + r.w - 16, r.y + r.h - 16, 16, 16 };
    if (!w->maximized) {
        if (ui_in(br, ms_x, ms_y))
            fill_rect(br.x, br.y, br.w, br.h, mixc(g_accent, rgb(0,0,0), 60));
        else
            fill_rect(br.x, br.y, br.w, br.h, rgb(52, 58, 72));
        // Draw diagonal lines
        draw_line(br.x + 3, br.y + 13, br.x + 13, br.y + 3, rgb(180, 188, 202));
        draw_line(br.x + 5, br.y + 13, br.x + 13, br.y + 5, rgb(120, 128, 144));
        draw_line(br.x + 7, br.y + 13, br.x + 13, br.y + 7, rgb(80, 88, 104));
    }

    if (l_released_now && ui_in(bc, ms_x, ms_y) && ui_in(bc, press_l_x, press_l_y)) {
        win_close(w);
        bump();
    } else if (l_released_now && ui_in(bm, ms_x, ms_y) && ui_in(bm, press_l_x, press_l_y)) {
        if (!w->maximized) {
            w->saved = w->r;
            w->r.x = 0; w->r.y = 0;
            w->r.w = SW; w->r.h = SH - TASKBAR_H;
            w->maximized = 1;
        } else {
            w->r = w->saved;
            w->maximized = 0;
        }
        bump();
    } else if (l_released_now && ui_in(bn, ms_x, ms_y) && ui_in(bn, press_l_x, press_l_y)) {
        if (w->anim == 0) { w->anim = 3; w->anim_tick = g_ticks; }
        bump();
    }
}

static void draw_taskbar(void)
{
    round_rect_blend(-16, SH - TASKBAR_H, SW + 32, TASKBAR_H + 32, 14, rgb(13, 14, 21), 222);
    for (int j = 0; j < TASKBAR_H; j++) {
        u32 c = mixc(rgb(28, 30, 41), rgb(12, 13, 20), (u8)(j * 255 / TASKBAR_H));
        blend_rect(0, SH - TASKBAR_H + j, SW, 1, c, 235);
    }
    blend_rect(0, SH - TASKBAR_H, SW, 1, mixc(g_accent, rgb(255, 255, 255), 55), 120);
    blend_rect(0, SH - TASKBAR_H + 1, SW, 1, mixc(g_accent, rgb(255, 255, 255), 30), 60);

    int group_w = 44 + N_PINS * 46;
    int gx = SW / 2 - group_w / 2;
    start_btn_r = (Rect){ gx, SH - TASKBAR_H + 6, 36, 36 };
    gx += 44;

    int over_start = ui_in(start_btn_r, ms_x, ms_y);
    if (over_start || start_open)
        round_rect_blend(start_btn_r.x - 3, start_btn_r.y - 3, start_btn_r.w + 6, start_btn_r.h + 6, 10, g_accent, 70);
    icon_draw(ICON_KLOGO, start_btn_r.x, start_btn_r.y, 36);

    glance_btn_r = (Rect){ SW - 252, SH - TASKBAR_H + 6, 36, 36 };
    int over_g = ui_in(glance_btn_r, ms_x, ms_y);
    if (over_g || glance_open)
        round_rect_blend(glance_btn_r.x - 3, glance_btn_r.y - 3, glance_btn_r.w + 6, glance_btn_r.h + 6, 10, g_accent, 70);
    circle_fill(glance_btn_r.x + 18, glance_btn_r.y + 18, 17, mixc(g_accent, rgb(18, 20, 28), 120));
    text(glance_btn_r.x + 14, glance_btn_r.y + 14, "K", 1, rgb(235, 238, 246));

    for (int i = 0; i < N_PINS; i++) {
        int app = pins[i];
        Window *w = win_by_app(app);
        Rect tr = { gx, SH - TASKBAR_H + 7, 36, 36 };
        int running = w && w->visible;
        int focused = running && w == g_focus;

        u32 tile = ui_hover_bg(rgb(30, 33, 43), tr);
        if (focused) tile = mixc(tile, g_accent, 35);
        round_rect(tr.x, tr.y, tr.w, tr.h, 9, tile);
        if (running) {
            blend_rect(tr.x + 4, tr.y + tr.h - 5, tr.w - 8, 2, g_accent, 200);
        }

        int id = app == APP_TERM ? ICON_TERM : app == APP_FILES ? ICON_FILES :
                 app == APP_CALC ? ICON_CALC : app == APP_DOODLE ? ICON_DOODLE :
                 app == APP_AV ? ICON_SHIELD : app == APP_EDIT ? ICON_TXT :
                 app == APP_SYSMON ? ICON_SETTINGS : app == APP_IMGVIEW ? ICON_IMAGE :
                 app == APP_MUSIC ? ICON_MUSIC : app == APP_SNAKE ? ICON_SNAKE :
                 app == APP_KALEIDOSCOPE ? ICON_DOODLE : app == APP_GOL ? ICON_DOODLE : ICON_SETTINGS;
        icon_draw(id, tr.x + 5, tr.y + 5, 26);
        gx += 46;
    }

    char tbuf[12], dbuf[16];
    format_time(tbuf, sizeof tbuf);
    format_date(dbuf, sizeof dbuf);
    int tx = SW - 206;
    text(tx, SH - TASKBAR_H + 10, tbuf, 1, rgb(235, 238, 245));
    text(tx, SH - TASKBAR_H + 24, dbuf, 1, rgb(140, 146, 160));
    icon_draw(ICON_SPEAKER, SW - 96, SH - TASKBAR_H + 15, 18);
    icon_draw(ICON_NET, SW - 70, SH - TASKBAR_H + 15, 18);
    icon_draw(ICON_BATTERY, SW - 44, SH - TASKBAR_H + 15, 18);
}

static int ci_sub(const char *hay, const char *ndl);

static void draw_start_menu(void)
{
    if (!start_open) return;

    int mw = 330;
    int items = N_START_ITEMS;
    int mh = 52 + 44 + items * 40 + 12 + 52;
    int mx = start_btn_r.x + start_btn_r.w / 2 - mw / 2;
    int my = SH - TASKBAR_H - mh - 10;
    if (mx < 8) mx = 8;

    round_rect_blend(mx + 4, my + 6, mw, mh, 14, rgb(0, 0, 0), 70);
    round_rect(mx, my, mw, mh, 14, rgb(19, 21, 30));
    for (int j = 0; j < mh; j++) {
        int li = 0;
        gfx_row_inset(j, mh, 14, &li);
        putpx(mx + li, my + j, g_accent);
        putpx(mx + mw - 1 - li, my + j, mixc(g_accent, rgb(0, 0, 0), 60));
    }

    icon_draw(ICON_KLOGO, mx + 14, my + 12, 28);
    text(mx + 50, my + 20, "KiKOS.11", 2, rgb(240, 243, 250));

    round_rect(mx + 16, my + 52, mw - 32, 34, 10, rgb(34, 37, 50));
    rect_outline(mx + 16, my + 52, mw - 31, 35, rgb(60, 66, 84));
    {
        int sxc = mx + 30, syc = my + 69;
        arc(sxc, syc, 7, 0, 360, rgb(150, 156, 172));
        draw_line(sxc + 5, syc + 5, sxc + 11, syc + 11, rgb(150, 156, 172));
    }
    {
        char sbuf[34];
        for (int i = 0; i < 32; i++) sbuf[i] = (i < start_search_len) ? start_search[i] : 0;
        sbuf[start_search_len] = 0;
        int typing = start_search_len > 0;
        text(mx + 50, my + 62, typing ? sbuf : "Search apps and files", 1,
             typing ? rgb(228, 232, 240) : rgb(120, 126, 140));
        if (typing && (g_ticks % 50 < 25)) {
            int cx = mx + 50 + text_w(sbuf, 1) + 1;
            vline(cx, my + 62, 8, rgb(228, 232, 240));
        }
    }

    int match[N_START_ITEMS], nm = 0;
    for (int i = 0; i < N_START_ITEMS; i++)
        if (start_search_len == 0 || ci_sub(start_items[i].label, start_search))
            match[nm++] = i;

    if (nm == 0)
        text(mx + 20, my + 130, "No results", 1, rgb(150, 156, 170));

    for (int k = 0; k < nm; k++) {
        int i = match[k];
        Rect ir = { mx + 10, my + 96 + k * 40, mw - 20, 36 };
        if (ui_in(ir, ms_x, ms_y))
            round_rect(ir.x, ir.y, ir.w, ir.h, 9, mixc(rgb(255, 255, 255), rgb(20, 22, 30), 225));
        int id = start_items[i].app == APP_TERM ? ICON_TERM :
                 start_items[i].app == APP_FILES ? ICON_FILES :
                 start_items[i].app == APP_CALC ? ICON_CALC :
                 start_items[i].app == APP_DOODLE ? ICON_DOODLE :
                 start_items[i].app == APP_AV ? ICON_SHIELD :
                 start_items[i].app == APP_EDIT ? ICON_TXT :
                 start_items[i].app == APP_SYSMON ? ICON_SETTINGS :
                 start_items[i].app == APP_IMGVIEW ? ICON_IMAGE :
                 start_items[i].app == APP_MUSIC ? ICON_MUSIC :
start_items[i].app == APP_SNAKE ? ICON_SNAKE :
                  start_items[i].app == APP_KALEIDOSCOPE ? ICON_DOODLE :
                  start_items[i].app == APP_GOL ? ICON_DOODLE :
                  start_items[i].app == APP_SETTINGS ? ICON_SETTINGS : ICON_ABOUT;
        icon_draw(id, ir.x + 7, ir.y + 7, 22);
        text(ir.x + 40, ir.y + 14, start_items[i].label, 1, rgb(228, 232, 240));

        if (clicked(ir)) {
            start_open = 0;
            start_search_len = 0;
            win_open(start_items[i].app);
        }
    }

    int py = my + mh - 52;
    Rect brb = { mx + 10, py, (mw - 26) / 2, 38 };
    Rect boff = { mx + 16 + (mw - 26) / 2, py, (mw - 26) / 2, 38 };

    round_rect(brb.x, brb.y, brb.w, brb.h, 9, ui_hover_bg(rgb(32, 35, 46), brb));
    icon_draw(ICON_REBOOT, brb.x + 10, brb.y + 11, 16);
    text(brb.x + 32, brb.y + 15, "Reboot", 1, rgb(225, 229, 238));

    round_rect(boff.x, boff.y, boff.w, boff.h, 9, ui_hover_bg(rgb(120, 44, 52), boff));
    icon_draw(ICON_POWER, boff.x + 10, boff.y + 11, 16);
    text(boff.x + 32, boff.y + 15, "Shut down", 1, rgb(245, 236, 238));

    if (clicked(brb)) reboot();
    if (clicked(boff)) power_off();
}

static int ci_eq(char a, char b)
{
    if (a == b) return 1;
    if (a >= 'A' && a <= 'Z' && a + 32 == b) return 1;
    if (b >= 'A' && b <= 'Z' && b + 32 == a) return 1;
    return 0;
}

static int ci_sub(const char *hay, const char *ndl)
{
    if (!*ndl) return 1;
    for (; *hay; hay++) {
        const char *h = hay, *n = ndl;
        int ok = 1;
        while (*h && *n) {
            if (!ci_eq(*h, *n)) { ok = 0; break; }
            h++; n++;
        }
        if (ok && !*n) return 1;
    }
    return 0;
}

int gui_handle_key(int key)
{
    if (!start_open) return 0;
    if (key == '\n' || key == '\r') {
        for (int i = 0; i < N_START_ITEMS; i++)
            if (start_search_len == 0 || ci_sub(start_items[i].label, start_search)) {
                start_open = 0; start_search_len = 0;
                win_open(start_items[i].app);
                break;
            }
        return 1;
    }
    if (key == 8 || key == 127) { if (start_search_len > 0) start_search_len--; return 1; }
    if (key >= 32 && key < 127) { if (start_search_len < 31) start_search[start_search_len++] = (char)key; return 1; }
    return 1;
}

static void compute_snap(Window *w)
{
    (void)w;
    snap_on = 0;
    int edge = 28, th = SH - TASKBAR_H;
    int qw = SW / 2, qh = (SH - TASKBAR_H) / 2;
    if (ms_x < edge && ms_y < edge) { snap_r = (Rect){ 0, 0, qw, qh }; snap_on = 3; return; }
    if (ms_x > SW - edge && ms_y < edge) { snap_r = (Rect){ qw, 0, qw, qh }; snap_on = 3; return; }
    if (ms_x < edge && ms_y > th - edge) { snap_r = (Rect){ 0, qh, qw, qh }; snap_on = 3; return; }
    if (ms_x > SW - edge && ms_y > th - edge) { snap_r = (Rect){ qw, qh, qw, qh }; snap_on = 3; return; }
    if (ms_y < edge) { snap_r = (Rect){ 0, 0, SW, th }; snap_on = 2; }
    else if (ms_x < edge) { snap_r = (Rect){ 0, 0, SW / 2, th }; snap_on = 1; }
    else if (ms_x > SW - edge) { snap_r = (Rect){ SW / 2, 0, SW - SW / 2, th }; snap_on = 1; }
}

static void apply_snap(Window *w)
{
    if (snap_on == 2) {
        w->saved = w->r;
        w->r.x = 0; w->r.y = 0; w->r.w = SW; w->r.h = SH - TASKBAR_H;
        w->maximized = 1;
    } else if (snap_on == 3) {
        w->maximized = 0;
        w->r = snap_r;
    } else {
        w->maximized = 0;
        w->r = snap_r;
    }
}

static void qs_layout(Rect *panel, Rect tiles[4], Rect *vol, Rect *bri)
{
    panel->x = SW - 320 - 12;
    panel->y = SH - TASKBAR_H - 12 - 284;
    panel->w = 320; panel->h = 284;
    int gx = panel->x + 16, gy = panel->y + 52;
    int tw = 132, th = 64, gap = 12;
    for (int i = 0; i < 4; i++) {
        tiles[i].x = gx + (i % 2) * (tw + gap);
        tiles[i].y = gy + (i / 2) * (th + gap);
        tiles[i].w = tw; tiles[i].h = th;
    }
    vol->x = panel->x + 16; vol->y = panel->y + 52 + 2 * (th + gap) + 10;
    vol->w = panel->w - 32; vol->h = 26;
    bri->x = vol->x; bri->y = vol->y + 44; bri->w = vol->w; bri->h = 26;
}

static void draw_quick_settings(void)
{
    Rect panel, tiles[4], vol, bri;
    qs_layout(&panel, tiles, &vol, &bri);

    round_rect_blend(panel.x + 4, panel.y + 6, panel.w, panel.h, 14, rgb(0, 0, 0), 70);
    round_rect(panel.x, panel.y, panel.w, panel.h, 14, rgb(20, 22, 30));
    text(panel.x + 18, panel.y + 20, "Quick Settings", 1, rgb(240, 243, 250));

    const char *names[4] = { "Wi-Fi", "Bluetooth", "Airplane", "Night light" };
    int states[4] = { qs_wifi, qs_bt, qs_air, g_nightlight };
    for (int i = 0; i < 4; i++) {
        int on = states[i];
        u32 base = on ? mixc(g_accent, rgb(20, 30, 26), 110) : rgb(34, 37, 48);
        round_rect(tiles[i].x, tiles[i].y, tiles[i].w, tiles[i].h, 10, base);
        if (on) hline(tiles[i].x, tiles[i].y, tiles[i].w, g_accent);
        else rect_outline(tiles[i].x, tiles[i].y, tiles[i].w + 1, tiles[i].h + 1, rgb(60, 66, 82));
        text(tiles[i].x + 12, tiles[i].y + 12, names[i], 1, rgb(235, 238, 246));
        text(tiles[i].x + 12, tiles[i].y + 34, on ? "On" : "Off", 1, on ? g_accent : rgb(150, 156, 170));
    }

    text(panel.x + 16, vol.y - 16, "Volume", 1, rgb(220, 225, 235));
    round_rect(vol.x, vol.y, vol.w, vol.h, 6, rgb(30, 33, 44));
    int vw = vol.w * qs_vol / 100;
    if (vw > 0) for (int j = 0; j < vol.h - 2; j++) hline(vol.x + 1, vol.y + 1 + j, vw - 2, g_accent);

    text(panel.x + 16, bri.y - 16, "Brightness", 1, rgb(220, 225, 235));
    round_rect(bri.x, bri.y, bri.w, bri.h, 6, rgb(30, 33, 44));
    int bw = bri.w * qs_bright / 100;
    if (bw > 0) for (int j = 0; j < bri.h - 2; j++) hline(bri.x + 1, bri.y + 1 + j, bw - 2, g_accent);
}

static void draw_ctx_menu(void)
{
    if (!ctx_open) return;
    static const char *labels[CTX_N] = { "New terminal", "Wallpaper...", "Refresh", "About KiKOS",
                                          "Command Palette", "Glance panel", "Night light", "Focus mode", "Mood" };

    int mw = ctx_r.w, mh = ctx_r.h;

    round_rect_blend(ctx_r.x + 3, ctx_r.y + 4, mw, mh, 10, rgb(0, 0, 0), 70);
    round_rect(ctx_r.x, ctx_r.y, mw, mh, 10, rgb(19, 21, 30));
    rect_outline(ctx_r.x, ctx_r.y, mw + 1, mh + 1, rgb(60, 66, 82));

    for (int i = 0; i < CTX_N; i++) {
        Rect ir = { ctx_r.x + 4, ctx_r.y + 4 + i * 30, mw - 8, 28 };
        if (ui_in(ir, ms_x, ms_y))
            round_rect(ir.x, ir.y, ir.w, ir.h, 7, mixc(rgb(255, 255, 255), rgb(20, 22, 30), 222));
        text(ir.x + 12, ir.y + 11, labels[i], 1, rgb(228, 232, 240));
        if (l_released_now && ui_in(ir, ms_x, ms_y) && ui_in(ir, press_l_x, press_l_y)) {
            ctx_open = 0;
            if (i == CTX_TERM) win_open(APP_TERM);
            else if (i == CTX_WALL) win_open(APP_SETTINGS);
            else if (i == CTX_ABOUT) win_open(APP_ABOUT);
            else if (i == CTX_PAL) { palette_open = 1; pal_len = 0; }
            else if (i == CTX_GLANCE) glance_open ^= 1;
            else if (i == CTX_NIGHT) { g_nightlight ^= 1; toast(g_nightlight ? "Night light on" : "Night light off"); }
            else if (i == CTX_FOCUS) { g_focus_mode ^= 1; toast(g_focus_mode ? "Focus mode on" : "Focus mode off"); }
            else if (i == CTX_MOOD) { g_mood ^= 1; toast(g_mood ? "Mood on" : "Mood off"); }
            else bump();
        }
    }
}

static void dispatch_mouse_to_apps(Window *w, int ev)
{
    Rect c = win_content(w);
    int lx = ms_x - c.x, ly = ms_y - c.y;
    switch (w->app) {
    case APP_TERM:     app_term_mouse(w, lx, ly, ev); break;
    case APP_FILES:    app_files_mouse(w, lx, ly, ev); break;
    case APP_CALC:     app_calc_mouse(w, lx, ly, ev); break;
    case APP_DOODLE:   app_doodle_mouse(w, lx, ly, ev); break;
    case APP_SETTINGS: app_settings_mouse(w, lx, ly, ev); break;
    case APP_ABOUT:    app_about_mouse(w, lx, ly, ev); break;
    case APP_AV:       app_av_mouse(w, lx, ly, ev); break;
    case APP_EDIT:     app_edit_mouse(w, lx, ly, ev); break;
    case APP_SYSMON:   app_sysmon_mouse(w, lx, ly, ev); break;
    case APP_IMGVIEW:  app_imgview_mouse(w, lx, ly, ev); break;
        case APP_MUSIC:    app_music_mouse(w, lx, ly, ev); break;
        case APP_SNAKE:    app_snake_mouse(w, lx, ly, ev); break;
        case APP_KALEIDOSCOPE: app_kaleido_mouse(w, lx, ly, ev); break;
        case APP_GOL:    app_gol_mouse(w, lx, ly, ev); break;
        case APP_SLIDE: app_slide_mouse(w, lx, ly, ev); break;
        }
    }

static const struct { int app; const char *label; int cmd; } pal_cmds[] = {
    { APP_TERM,    "Terminal",        0 },
    { APP_FILES,   "Files",           0 },
    { APP_CALC,    "Calculator",      0 },
    { APP_DOODLE,  "Doodle",          0 },
    { APP_AV,      "Antivirus",       0 },
    { APP_EDIT,    "Text Editor",     0 },
    { APP_SYSMON,  "System Monitor",  0 },
    { APP_IMGVIEW, "Image Viewer",    0 },
    { APP_MUSIC,   "Music Player",    0 },
    { APP_SETTINGS,"Settings",        0 },
    { APP_ABOUT,   "About KiKOS",     0 },
    { APP_SNAKE,   "Snake",           0 },
    { APP_KALEIDOSCOPE, "Kaleidoscope", 0 },
    { APP_GOL,     "Game of Life",    0 },
    { APP_SLIDE,   "Slide Puzzle",    0 },
    { 0, "Night light",     1 },
    { 0, "Focus mode",      2 },
    { 0, "Reboot",          4 },
    { 0, "Shut down",       5 },
    { 0, "Wallpaper: Aurora", 6 },
    { 0, "Wallpaper: Sunset", 7 },
    { 0, "Wallpaper: Ocean",  8 },
    { 0, "Wallpaper: Mono",   9 },
    { 0, "Glance panel",      10 },
};
#define N_PAL_CMDS (int)(sizeof(pal_cmds) / sizeof(pal_cmds[0]))

static int pal_matches(int out[])
{
    int n = 0;
    for (int i = 0; i < N_PAL_CMDS; i++)
        if (pal_len == 0 || ci_sub(pal_cmds[i].label, pal_buf))
            out[n++] = i;
    return n;
}

static void pal_launch(int idx)
{
    int app = pal_cmds[idx].app;
    int cmd = pal_cmds[idx].cmd;
    palette_open = 0; pal_len = 0;
    if (app) { win_open(app); bump(); return; }
    switch (cmd) {
        case 1: g_nightlight ^= 1; toast(g_nightlight ? "Night light on" : "Night light off"); break;
        case 2: g_focus_mode ^= 1; toast(g_focus_mode ? "Focus mode on" : "Focus mode off"); break;
        case 4: reboot(); return;
        case 5: power_off(); return;
        case 6: theme_set(WP_AURORA); toast("Wallpaper: Aurora"); break;
        case 7: theme_set(WP_SUNSET); toast("Wallpaper: Sunset"); break;
        case 8: theme_set(WP_OCEAN); toast("Wallpaper: Ocean"); break;
        case 9: theme_set(WP_MONO); toast("Wallpaper: Mono"); break;
        case 10: glance_open = !glance_open; toast(glance_open ? "Glance open" : "Glance closed"); break;
    }
    bump();
}

static void pal_rects(Rect *panel, Rect rows[12])
{
    panel->w = 560; panel->h = 360;
    panel->x = SW / 2 - panel->w / 2;
    panel->y = SH / 2 - panel->h / 2 - 40;
    for (int k = 0; k < 12; k++)
        rows[k] = (Rect){ panel->x + 16, panel->y + 72 + k * 38, panel->w - 32, 34 };
}

static void draw_command_palette(void)
{
    Rect panel, rows[12];
    pal_rects(&panel, rows);
    round_rect_blend(panel.x + 6, panel.y + 8, panel.w, panel.h, 16, rgb(0, 0, 0), 90);
    round_rect(panel.x, panel.y, panel.w, panel.h, 16, rgb(18, 20, 28));
    rect_outline(panel.x, panel.y, panel.w + 1, panel.h + 1, mixc(g_accent, rgb(255,255,255), 50));

    round_rect(panel.x + 16, panel.y + 16, panel.w - 32, 40, 10, rgb(30, 33, 44));
    {
        char buf[50];
        for (int i = 0; i < 48; i++) buf[i] = (i < pal_len) ? pal_buf[i] : 0;
        buf[pal_len] = 0;
        int typing = pal_len > 0;
        text(panel.x + 28, panel.y + 30, typing ? buf : "Type a command or app...", 1,
             typing ? rgb(235, 238, 246) : rgb(130, 136, 150));
        if (typing && (g_ticks % 50 < 25)) {
            int cx = panel.x + 28 + text_w(buf, 1) + 1;
            vline(cx, panel.y + 28, 12, rgb(235, 238, 246));
        }
    }

    int m[24]; int n = pal_matches(m);
    int maxr = (panel.h - 80) / 38;
    for (int k = 0; k < n && k < maxr; k++) {
        int i = m[k];
        Rect ir = rows[k];
        int sel = (k == 0);
        if (sel) round_rect(ir.x, ir.y, ir.w, ir.h, 8, mixc(g_accent, rgb(20, 22, 30), 120));
        else if (ui_in(ir, ms_x, ms_y)) round_rect(ir.x, ir.y, ir.w, ir.h, 8, rgb(30, 33, 44));
        int id = pal_cmds[i].app == APP_TERM ? ICON_TERM :
                 pal_cmds[i].app == APP_FILES ? ICON_FILES :
                 pal_cmds[i].app == APP_CALC ? ICON_CALC :
                 pal_cmds[i].app == APP_DOODLE ? ICON_DOODLE :
                 pal_cmds[i].app == APP_AV ? ICON_SHIELD :
                 pal_cmds[i].app == APP_EDIT ? ICON_TXT :
                 pal_cmds[i].app == APP_SYSMON ? ICON_SETTINGS :
                 pal_cmds[i].app == APP_IMGVIEW ? ICON_IMAGE :
                 pal_cmds[i].app == APP_MUSIC ? ICON_MUSIC :
pal_cmds[i].app == APP_SNAKE ? ICON_SNAKE :
                  pal_cmds[i].app == APP_KALEIDOSCOPE ? ICON_DOODLE :
                  pal_cmds[i].app == APP_GOL ? ICON_DOODLE : ICON_SETTINGS;
        icon_draw(id, ir.x + 8, ir.y + 6, 22);
        text(ir.x + 40, ir.y + 11, pal_cmds[i].label, 1, rgb(228, 232, 240));
        if (sel) text(ir.x + ir.w - 44, ir.y + 11, "Enter", 1, g_accent);
    }
    if (n == 0) text(panel.x + 28, panel.y + 96, "No matching commands", 1, rgb(140, 146, 160));
}

static void aura_dot(int x, int y, int r, u32 c, u8 a)
{
    for (int j = -r; j <= r; j++)
        for (int i = -r; i <= r; i++) {
            int d2 = i * i + j * j;
            if (d2 > r * r) continue;
            int px = x + i, py = y + j;
            if (px < 0 || px >= SW || py < 0 || py >= SH) continue;
            u32 cur = getpx(px, py);
            u8 fa = (u8)(a * (u32)(r * r - d2) / (r * r));
            putpx(px, py, mixc(cur, c, fa));
        }
}

int gui_handle_palette_key(int key)
{
    if (!palette_open) return 0;
    if (key == 27) { palette_open = 0; pal_len = 0; return 1; }
    if (key == '\n' || key == '\r') {
        int m[24]; int n = pal_matches(m);
        if (n > 0) pal_launch(m[0]);
        return 1;
    }
    if (key == 8 || key == 127) { if (pal_len > 0) pal_len--; return 1; }
    if (key >= 32 && key < 127) { if (pal_len < 47) pal_buf[pal_len++] = (char)key; return 1; }
    return 1;
}

int gui_palette_toggle(int key)
{
    if (key == 11) { palette_open = !palette_open; if (palette_open) pal_len = 0; bump(); return 1; }
    return 0;
}

int gui_handle_fkey(int key)
{
    if (key < 0xA1 || key > 0xAA) return 0;
    switch (key) {
        case 0xA1: palette_open = !palette_open; if (palette_open) pal_len = 0; break;
        case 0xA2: g_nightlight ^= 1; toast(g_nightlight ? "Night light on" : "Night light off"); break;
        case 0xA3: g_focus_mode ^= 1; toast(g_focus_mode ? "Focus mode on" : "Focus mode off"); break;
        case 0xA4: g_mood ^= 1; toast(g_mood ? "Mood on" : "Mood off"); break;
        case 0xA5: glance_open = !glance_open; toast(glance_open ? "Glance open" : "Glance closed"); break;
        case 0xA6: qs_open = !qs_open; break;
        case 0xA7: g_auto_theme ^= 1; toast(g_auto_theme ? "Adaptive theme on" : "Adaptive theme off"); break;
        case 0xA8:
            g_theme = (g_theme + 1) % 4; theme_set(g_theme);
            toast(g_theme == WP_AURORA ? "Wallpaper: Aurora" : g_theme == WP_SUNSET ? "Wallpaper: Sunset" :
                  g_theme == WP_OCEAN ? "Wallpaper: Ocean" : "Wallpaper: Mono");
            break;
        default: return 1;
    }
    bump();
    return 1;
}

static u32 hsv2rgb(u32 h, u32 s, u32 v)
{
    h %= 360;
    u32 c = (v * s) / 255;
    u32 seg = h / 60;
    u32 f = h % 60;
    u32 x = c * (seg % 2 ? f : (60 - f)) / 60;
    u32 m = v - c;
    u32 r, g, b;
    switch (seg) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }
    return rgb(r + m, g + m, b + m);
}

void toast(const char *m)
{
    int idx = -1;
    for (int i = 0; i < N_TOAST; i++) if (!toasts[i].on) { idx = i; break; }
    if (idx < 0) {
        idx = 0; u32 old = 0xffffffff;
        for (int i = 0; i < N_TOAST; i++) if (toasts[i].born < old) { old = toasts[i].born; idx = i; }
    }
    strncpy(toasts[idx].msg, m, 63); toasts[idx].msg[63] = 0;
    toasts[idx].born = g_ticks;
    toasts[idx].on = 1;
    bump();
}

static void glance_rects(Rect *panel, Rect pills[4])
{
    panel->x = glance_x; panel->y = 0; panel->w = 320; panel->h = SH - TASKBAR_H;
    int px = panel->x + 24;
    for (int i = 0; i < 4; i++)
        pills[i] = (Rect){ px, 250 + i * 50, panel->w - 48, 40 };
}

static void draw_glance(void)
{
    if (glance_x >= SW - 4) return;
    Rect panel, pills[4];
    glance_rects(&panel, pills);

    round_rect(panel.x, panel.y, panel.w, panel.h, 0, rgb(16, 18, 26));
    rect_outline(panel.x, panel.y, panel.w + 1, panel.h + 1, mixc(g_accent, rgb(255, 255, 255), 60));

    circle_fill(panel.x + 34, 40, 20, g_accent);
    text(panel.x + 26, 30, "K", 2, rgb(255, 255, 255));
    text(panel.x + 70, 26, "Glance", 2, rgb(240, 243, 250));

    char tbuf[12], dbuf[16];
    format_time(tbuf, sizeof tbuf);
    format_date(dbuf, sizeof dbuf);
    text(panel.x + 24, 84, tbuf, 3, rgb(245, 248, 255));
    text(panel.x + 24, 138, dbuf, 1, rgb(150, 156, 172));

    u32 used = heap_used() / 1024, total = heap_total() / 1024;
    u32 pct = used * 100 / total;
    text(panel.x + 24, 178, "Memory", 1, rgb(200, 206, 220));
    round_rect(panel.x + 24, 198, panel.w - 48, 10, 5, rgb(30, 33, 46));
    if (pct > 0) round_rect(panel.x + 24, 198, (panel.w - 48) * pct / 100, 10, 5, g_accent);
    char mb[40]; strcpy(mb, ""); utoa_dec(used, mb); strcat(mb, " / ");
    char tb[16]; utoa_dec(total, tb); strcat(mb, tb); strcat(mb, " MB");
    text(panel.x + 24, 214, mb, 1, rgb(150, 156, 172));

    char ub[32]; strcpy(ub, "Uptime ");
    u32 s = uptime_ms() / 1000; u32 mm = s / 60, ss = s % 60;
    char n1[8], n2[8]; utoa_dec(mm, n1); utoa_dec(ss, n2);
    strcat(ub, n1); strcat(ub, ":"); strcat(ub, n2);
    text(panel.x + 24, 232, ub, 1, rgb(150, 156, 172));

    const char *lbl[4] = {
        g_nightlight ? "Night light: On" : "Night light: Off",
        g_focus_mode ? "Focus mode: On"  : "Focus mode: Off",
        g_mood ? "Mood: On" : "Mood: Off",
        qs_wifi ? "Wi-Fi: On" : "Wi-Fi: Off",
    };
    for (int i = 0; i < 4; i++) {
        Rect pr = pills[i];
        int hov = ui_in(pr, ms_x, ms_y);
        round_rect(pr.x, pr.y, pr.w, pr.h, 9, hov ? mixc(rgb(255,255,255), rgb(20,22,30), 40) : rgb(26, 28, 38));
        rect_outline(pr.x, pr.y, pr.w + 1, pr.h + 1, hov ? g_accent : rgb(64, 70, 86));
        text(pr.x + 14, pr.y + 13, lbl[i], 1, rgb(228, 232, 240));
    }

    Rect cls = { panel.x + panel.w - 40, 16, 28, 28 };
    if (ui_in(cls, ms_x, ms_y)) round_rect(cls.x, cls.y, cls.w, cls.h, 6, rgb(46, 49, 60));
    draw_line(cls.x + 9, cls.y + 9, cls.x + 19, cls.y + 19, rgb(220, 224, 234));
    draw_line(cls.x + 19, cls.y + 9, cls.x + 9, cls.y + 19, rgb(220, 224, 234));
}

static void draw_toasts(void)
{
    int y = 16;
    for (int i = 0; i < N_TOAST; i++) {
        if (!toasts[i].on) continue;
        u32 age = g_ticks - toasts[i].born;
        if (age > 220) { toasts[i].on = 0; continue; }
        int tw = text_w(toasts[i].msg, 1) + 60;
        int slide = age < 12 ? (12 - (int)age) * tw / 12 : 0;
        int tx = SW - 16 - tw + slide;
        round_rect(tx, y, tw, 44, 10, rgb(24, 26, 36));
        rect_outline(tx, y, tw + 1, 45, mixc(g_accent, rgb(255, 255, 255), 60));
        circle_fill(tx + 22, y + 22, 12, g_accent);
        text(tx + 16, y + 16, "K", 1, rgb(255, 255, 255));
        text(tx + 44, y + 15, toasts[i].msg, 1, rgb(235, 238, 246));
        y += 52;
    }
}

static void handle_input(void)
{
    int cur_l = ms_btn_l;
    int cur_r = ms_btn_r;
    l_pressed_now = cur_l && !prev_l;
    l_released_now = !cur_l && prev_l;
    r_pressed_now = cur_r && !prev_r;
    if (l_pressed_now) { press_l_x = ms_x; press_l_y = ms_y; }

    if (r_pressed_now) {
        int over_win = 0;
        for (int i = g_nwins - 1; i >= 0; i--) {
            if (g_wins[i].visible && ui_in(g_wins[i].r, ms_x, ms_y)) { over_win = 1; break; }
        }
        int over_bar = ms_y >= SH - TASKBAR_H;
        if (!over_win && !over_bar) {
            ctx_open = 1;
            start_open = 0;
            ctx_r.w = 190;
            ctx_r.h = CTX_N * 30 + 8;
            ctx_r.x = ms_x;
            ctx_r.y = ms_y;
            if (ctx_r.x + ctx_r.w > SW - 8) ctx_r.x = SW - ctx_r.w - 8;
            if (ctx_r.y + ctx_r.h > SH - TASKBAR_H - 8) ctx_r.y -= ctx_r.h + 8;
            if (ctx_r.y < 4) ctx_r.y = 4;
            bump();
            goto done;
        }
    }

    if (ctx_open && l_pressed_now &&
        !(ms_x >= ctx_r.x && ms_x < ctx_r.x + ctx_r.w &&
          ms_y >= ctx_r.y && ms_y < ctx_r.y + ctx_r.h)) {
        ctx_open = 0;
        bump();
    }

    if (start_open && l_pressed_now) {
        int inside = ms_y >= SH - TASKBAR_H - 420 && ms_y <= SH &&
                     ms_x >= start_btn_r.x - 160 && ms_x <= start_btn_r.x + 380;
        if (!inside && !(ui_in(start_btn_r, ms_x, ms_y))) {
            start_open = 0;
            start_search_len = 0;
            bump();
        }
    }

    if (qs_open && l_pressed_now) {
        Rect panel, tiles[4], vol, bri;
        qs_layout(&panel, tiles, &vol, &bri);
        if (ui_in(panel, ms_x, ms_y)) {
            for (int i = 0; i < 4; i++) {
                if (clicked(tiles[i])) {
                    if (i == 0) { qs_wifi ^= 1; toast(qs_wifi ? "Wi-Fi on" : "Wi-Fi off"); }
                    else if (i == 1) { qs_bt ^= 1; toast(qs_bt ? "Bluetooth on" : "Bluetooth off"); }
                    else if (i == 2) { qs_air ^= 1; toast(qs_air ? "Airplane mode on" : "Airplane mode off"); }
                    else { g_nightlight ^= 1; toast(g_nightlight ? "Night light on" : "Night light off"); }
                    bump();
                    goto done;
                }
            }
            if (clicked(vol)) {
                qs_vol = (ms_x - vol.x) * 100 / vol.w;
                if (qs_vol < 0) qs_vol = 0;
                if (qs_vol > 100) qs_vol = 100;
                bump(); goto done;
            }
            if (clicked(bri)) {
                qs_bright = (ms_x - bri.x) * 100 / bri.w;
                if (qs_bright < 0) qs_bright = 0;
                if (qs_bright > 100) qs_bright = 100;
                bump(); goto done;
            }
            goto done;
        } else if (!(ms_x >= SW - 230 && ms_y >= SH - TASKBAR_H)) {
            qs_open = 0;
            bump();
        }
    }

    if (palette_open && l_pressed_now) {
        Rect panel, rows[12];
        pal_rects(&panel, rows);
        if (ui_in(panel, ms_x, ms_y)) {
            int m[24]; int n = pal_matches(m);
            int maxr = (panel.h - 80) / 38;
            for (int k = 0; k < n && k < maxr; k++)
                if (clicked(rows[k])) { pal_launch(m[k]); goto done; }
            goto done;
        } else {
            palette_open = 0; pal_len = 0; bump();
        }
    }

    if (glance_open && l_pressed_now) {
        Rect panel, pills[4];
        glance_rects(&panel, pills);
        if (ui_in(panel, ms_x, ms_y)) {
            Rect cls = { panel.x + panel.w - 40, 16, 28, 28 };
            if (clicked(cls)) { glance_open = 0; bump(); goto done; }
            for (int i = 0; i < 4; i++) {
                if (clicked(pills[i])) {
                    if (i == 0) { g_nightlight ^= 1; toast(g_nightlight ? "Night light on" : "Night light off"); }
                    else if (i == 1) { g_focus_mode ^= 1; toast(g_focus_mode ? "Focus mode on" : "Focus mode off"); }
                    else if (i == 2) { g_mood ^= 1; toast(g_mood ? "Mood on" : "Mood off"); }
                    else { qs_wifi ^= 1; toast(qs_wifi ? "Wi-Fi on" : "Wi-Fi off"); }
                    bump(); goto done;
                }
            }
            goto done;
        } else if (!ui_in(glance_btn_r, ms_x, ms_y)) {
            glance_open = 0; bump();
        }
    }

    if (drag_win) {
        if (cur_l) {
            drag_win->r.x = ms_x - drag_offx;
            drag_win->r.y = ms_y - drag_offy;
            if (drag_win->r.y < 0) drag_win->r.y = 0;
            if (drag_win->r.y > SH - TASKBAR_H - TITLE_H) drag_win->r.y = SH - TASKBAR_H - TITLE_H;
            if (drag_win->r.x < -drag_win->r.w + 80) drag_win->r.x = -drag_win->r.w + 80;
            if (drag_win->r.x > SW - 80) drag_win->r.x = SW - 80;
            compute_snap(drag_win);
        } else {
            if (snap_on) apply_snap(drag_win);
            snap_on = 0;
            drag_win = 0;
        }
    }

    if (resize_win) {
        if (cur_l) {
            int new_w = ms_x - resize_win->r.x + resize_offx;
            int new_h = ms_y - resize_win->r.y + resize_offy;
            if (new_w < 160) new_w = 160;
            if (new_h < 100) new_h = 100;
            if (resize_win->r.x + new_w > SW) new_w = SW - resize_win->r.x;
            if (resize_win->r.y + new_h > SH - TASKBAR_H) new_h = SH - TASKBAR_H - resize_win->r.y;
            resize_win->r.w = new_w;
            resize_win->r.h = new_h;
        } else {
            resize_win = 0;
        }
    }

    if (l_pressed_now) {
        Window *hit = 0;
        for (int i = g_nwins - 1; i >= 0; i--) {
            if (g_wins[i].visible && ui_in(g_wins[i].r, ms_x, ms_y)) { hit = &g_wins[i]; break; }
        }
        if (!hit && ms_y >= SH - TASKBAR_H) {
            if (ui_in(start_btn_r, ms_x, ms_y)) {
                start_open = !start_open;
                if (start_open) start_search_len = 0;
                bump();
                goto done;
            }
            int gx = SW / 2 - (44 + N_PINS * 46) / 2 + 44;
            for (int i = 0; i < N_PINS; i++) {
                Rect tr = { gx, SH - TASKBAR_H + 7, 36, 36 };
                if (ui_in(tr, ms_x, ms_y)) {
                    int app = pins[i];
                    Window *w = win_by_app(app);
                    if (w && w->visible) {
                        if (w == g_focus || w->anim == 3) {
                            w->visible = 0;
                            w->anim = 0;
                            g_focus = 0;
                            for (int j = g_nwins - 1; j >= 0; j--)
                                if (g_wins[j].visible) { g_focus = &g_wins[j]; break; }
                        }
                        else focus_win(w);
                    } else {
                        win_open(app);
                    }
                    bump();
                    goto done;
                }
                gx += 46;
            }
            if (ui_in(glance_btn_r, ms_x, ms_y)) { glance_open = !glance_open; bump(); goto done; }
            if (ms_x >= SW - 230) { qs_open = !qs_open; bump(); goto done; }
            goto done;
        }
        if (hit) {
            focus_win(hit);
    int bw = 26;
    Rect bc = { hit->r.x + hit->r.w - bw - 8, hit->r.y + 6, bw, bw };
    Rect bm = { hit->r.x + hit->r.w - 2 * bw - 12, hit->r.y + 6, bw, bw };
    Rect bn = { hit->r.x + hit->r.w - 3 * bw - 16, hit->r.y + 6, bw, bw };
            Rect br = { hit->r.x + hit->r.w - 16, hit->r.y + hit->r.h - 16, 16, 16 };
            if (!hit->maximized && ui_in(br, ms_x, ms_y)) {
                resize_win = hit;
                resize_offx = hit->r.w - (ms_x - hit->r.x);
                resize_offy = hit->r.h - (ms_y - hit->r.y);
            } else if (!ui_in(bc, ms_x, ms_y) && !ui_in(bm, ms_x, ms_y) && !ui_in(bn, ms_x, ms_y)) {
                if (ms_y < hit->r.y + TITLE_H) {
                    if (hit->maximized) { hit->r = hit->saved; hit->maximized = 0; }
                    drag_win = hit;
                    drag_offx = ms_x - hit->r.x;
                    drag_offy = ms_y - hit->r.y;
                } else if (ms_y >= win_content(hit).y) {
                    dispatch_mouse_to_apps(hit, ME_PRESS);
                }
            }
            goto done;
        }
        for (int i = 0; i < N_DESK_ICONS; i++) {
            Rect ir = { 20, 16 + i * 92, 84, 84 };
            if (ui_in(ir, ms_x, ms_y)) {
                desk_sel = i;
                int now = (int)g_ticks;
                if (last_click_icon == i && now - last_click_tick < 40) {
                    int app = i == 0 ? APP_TERM : i == 1 ? APP_FILES :
                              i == 2 ? APP_DOODLE : i == 3 ? APP_AV :
                              i == 4 ? APP_EDIT : i == 5 ? APP_SYSMON :
                              i == 6 ? APP_IMGVIEW : i == 7 ? APP_MUSIC :
                              i == 8 ? APP_ABOUT : i == 9 ? APP_SNAKE :
                              i == 10 ? APP_GOL : APP_SLIDE;
                    win_open(app);
                    last_click_icon = -1;
                } else {
                    last_click_icon = i;
                    last_click_tick = now;
                }
                bump();
                goto done;
            }
        }
        if (desk_sel != -1) { desk_sel = -1; bump(); }
    }

    if ((ms_btn_l || l_released_now) && g_focus && g_focus->visible) {
        Window *top = 0;
        for (int i = g_nwins - 1; i >= 0; i--)
            if (g_wins[i].visible) { top = &g_wins[i]; break; }
        if (top == g_focus && ui_in(g_focus->r, ms_x, ms_y)) {
            Rect c = win_content(g_focus);
            if (ms_y >= c.y)
                dispatch_mouse_to_apps(g_focus,
                                       l_pressed_now ? ME_PRESS :
                                       l_released_now ? ME_RELEASE : ME_MOVE);
        }
    }

done:
    prev_l = cur_l;
    prev_r = cur_r;
}

static void screensaver_draw(void)
{
    if (!stars_inited) {
        for (int i = 0; i < N_STARS; i++) {
            stars[i].x = (int)(rand32() % (u32)SW);
            stars[i].y = (int)(rand32() % (u32)SH);
            stars[i].z = 1 + (int)(rand32() % (u32)SW);
        }
        stars_inited = 1;
    }

    blend_rect(0, 0, SW, SH, rgb(2, 4, 14), 215);

    for (int i = 0; i < N_STARS; i++) {
        stars[i].z -= 3;
        if (stars[i].z < 1) {
            stars[i].x = (int)(rand32() % (u32)SW);
            stars[i].y = (int)(rand32() % (u32)SH);
            stars[i].z = SW;
            continue;
        }
        int px = ((stars[i].x - SW / 2) * 256) / stars[i].z;
        int py = ((stars[i].y - SH / 2) * 256) / stars[i].z;
        int sx = SW / 2 + px;
        int sy = SH / 2 + py;
        if (sx < 0 || sx >= SW || sy < 0 || sy >= SH) continue;
        int b = 255 - (stars[i].z * 255) / SW;
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        u32 color = mixc(g_accent, rgb(255, 255, 255), (u8)(b * 255 / 255));
        putpx(sx, sy, mixc(color, rgb(255, 255, 255), (u8)(b / 2)));
    }

    // Subtle aurora glow orbs during screensaver
    int t = (int)(g_ticks);
    blend_rect(SW * 2 / 3 + (int)((t / 3) % 160) - 80, SH / 3, 160, 160, g_accent_a, 18);
    blend_rect(SW / 5 + (int)((t * 2 / 5) % 220) - 110, SH * 3 / 4, 160, 160, g_accent_b, 18);

    const char *msg = "KiKOS.11  -  move the mouse or press a key to wake";
    text(SW / 2 - text_w(msg, 2) / 2, SH / 2 - 8, msg, 2, rgb(220, 230, 255));
}

static void draw_desktop_clock(void)
{
    int wx = SW - 172, wy = 20, ww = 152, wh = 150;

    round_rect_blend(wx + 3, wy + 5, ww, wh, 14, rgb(0, 0, 0), 72);
    round_rect(wx, wy, ww, wh, 14, rgb(17, 19, 27));
    for (int j = 0; j < wh; j++) {
        int li = 0;
        gfx_row_inset(j, wh, 14, &li);
        putpx(wx + li, wy + j, mixc(g_accent, rgb(0, 0, 0), 110));
        putpx(wx + ww - 1 - li, wy + j, mixc(g_accent, rgb(0, 0, 0), 140));
    }

    int cx = wx + ww / 2, cy = wy + 52, R = 34;
    circle_fill(cx, cy, R + 2, rgb(22, 24, 33));
    circle_fill(cx, cy, R, rgb(9, 10, 16));
    for (int t = 0; t < 60; t++) {
        int x0, y0, x1, y1;
        arc_point(cx, cy, R - 1, t * 6, &x0, &y0);
        arc_point(cx, cy, (t % 5 == 0) ? R - 5 : R - 3, t * 6, &x1, &y1);
        draw_line(x0, y0, x1, y1, (t % 5 == 0) ? rgb(230, 234, 244) : rgb(90, 96, 112));
    }

    int sec = g_rtc.sec, min = g_rtc.min, hour = g_rtc.hour % 12;
    int secd = (sec * 6) % 360;
    int mind = (min * 6 + sec / 10) % 360;
    int hourd = (hour * 30 + min / 2) % 360;

    int hx, hy;
    arc_point(cx, cy, R - 12, hourd, &hx, &hy);
    draw_line(cx, cy, hx, hy, rgb(245, 248, 255));
    arc_point(cx, cy, R - 4, mind, &hx, &hy);
    draw_line(cx, cy, hx, hy, rgb(210, 216, 230));

    arc_point(cx, cy, R - 2, secd, &hx, &hy);
    draw_line(cx, cy, hx, hy, g_accent);
    circle_fill(cx, cy, 2, g_accent);

    char tbuf[12], dbuf[16];
    format_time(tbuf, sizeof tbuf);
    format_date(dbuf, sizeof dbuf);
    text(wx + ww / 2 - text_w(tbuf, 1) / 2, wy + 96, tbuf, 1, rgb(235, 238, 246));
    text(wx + ww / 2 - text_w(dbuf, 1) / 2, wy + 110, dbuf, 1, rgb(140, 146, 160));

    u32 used = heap_used() / 1024, total = heap_total() / 1024;
    u32 pct = used * 100 / total;
    round_rect(wx + 12, wy + 128, ww - 24, 7, 3, rgb(30, 33, 46));
    if (pct > 0) round_rect(wx + 12, wy + 128, (ww - 24) * pct / 100, 7, 3, mixc(g_accent, rgb(255, 255, 255), 60));
}

void gui_frame(void)
{
    rtc_poll();
    if (g_auto_theme) {
        static int auto_period = -1;
        int h = g_rtc.hour;
        int period = (h < 7 || h >= 19) ? 1 : 0;
        if (period != auto_period) {
            auto_period = period;
            theme_set(period ? WP_OCEAN : WP_AURORA);
            g_nightlight = period;
        }
    }    if (g_mood) {
        static u32 mood_hue = 0;
        mood_hue = (mood_hue + 1) % 360;
        g_accent = hsv2rgb(mood_hue, 170, 255);
        g_accent_a = hsv2rgb((mood_hue + 40) % 360, 190, 255);
        g_accent_b = hsv2rgb((mood_hue + 200) % 360, 190, 255);
        static u32 mood_re = 0;
        if (g_ticks - mood_re > 24) { mood_re = g_ticks; wall_render(g_theme); }
    }
    {
        static int ginit = 0;
        if (!ginit) { ginit = 1; glance_x = SW; }
        int target = glance_open ? SW - 320 : SW;
        glance_x += (target - glance_x) / 6;
        if ((target - glance_x < 0 ? glance_x - target : target - glance_x) < 2) glance_x = target;
    }
    handle_input();

    static u32 prev_epoch = 0;
    if (g_input_epoch != prev_epoch) {
        prev_epoch = g_input_epoch;
        last_input_tick = g_ticks;
        ss_on = 0;
    }
    if (g_gui_active && (g_ticks - last_input_tick) > SS_IDLE_TICKS)
        ss_on = 1;


    wall_blit();

    {
        u32 t = g_ticks;
        int gx1 = SW * 4 / 5 + (int)(t % 200) - 100;
        int gy1 = SH / 5 + (int)((t * 7) % 180) - 90;
        u8 ga1 = (u8)(18 + (int)((t / 4) % 20));
        blend_rect(gx1 - 60, gy1 - 60, 120, 120, g_accent_a, ga1);

        int gx2 = SW / 6 + (int)((t * 3) % 220) - 110;
        int gy2 = SH * 9 / 10 + (int)((t * 5) % 160) - 80;
        u8 ga2 = (u8)(15 + (int)((t / 5) % 18));
        blend_rect(gx2 - 50, gy2 - 50, 100, 100, g_accent_b, ga2);
    }

    for (int k = trail_n - 1; k >= 0; k--) {
        int r = 7 - k / 4; if (r < 2) r = 2;
        u8 a = (u8)(70 * (trail_n - k) / trail_n);
        aura_dot(trail_x[k], trail_y[k], r, g_accent, a);
    }
    for (int k = trail_n - 1; k > 0; k--) { trail_x[k] = trail_x[k - 1]; trail_y[k] = trail_y[k - 1]; }
    if (trail_n < 24) trail_n++;
    trail_x[0] = ms_x; trail_y[0] = ms_y;

    draw_desktop_icons();

    if (!ss_on) draw_desktop_clock();

    for (int i = 0; i < g_nwins; i++) {
        Window *w = &g_wins[i];
        if (!w->visible) continue;
        Rect saved_r = w->r;
        int off = 0, a = 255;

        if (w->anim == 2) { /* closing: fade + slide down */
            int dur = (int)(g_ticks - w->anim_tick);
            if (dur < 10) {
                off = dur * 4;
                a = 255 - dur * 255 / 10;
                if (a < 10) a = 10;
            } else {
                win_close_finish(w);
                w = 0;
            }
        } else if (w->anim == 1) { /* opening: slide up + fade in */
            int dur = (int)(g_ticks - w->anim_tick);
            if (dur < 12) { off = (12 - dur) * 3; a = dur * 255 / 12; if (a < 12) a = 12; }
            else w->anim = 0;
        } else if (w->anim == 3) { /* minimizing: shrink + fade to taskbar */
            int dur = (int)(g_ticks - w->anim_tick);
            if (dur < 10) {
                int p = dur * 100 / 10;
                int neww = w->r.w * (100 - p) / 100;
                int newh = w->r.h * (100 - p) / 100;
                if (neww < 4) neww = 4;
                if (newh < 4) newh = 4;
                w->r.x += (w->r.w - neww) / 2;
                w->r.y += (w->r.h - newh) / 2;
                w->r.w = neww; w->r.h = newh;
                a = 255 - p * 255 / 100;
                if (a < 10) a = 10;
                off = 0;
            } else {
                w->visible = 0;
                w->anim = 0;
            }
        }

        if (!w) continue;
        w->r.y += off;
        if (w->r.y < 0) w->r.y = 0;
        draw_window_chrome(w);
        Rect c = win_content(w);
        switch (w->app) {
        case APP_TERM:     app_term_draw(w, &c); break;
        case APP_FILES:    app_files_draw(w, &c); break;
        case APP_CALC:     app_calc_draw(w, &c); break;
        case APP_DOODLE:   app_doodle_draw(w, &c); break;
        case APP_SETTINGS: app_settings_draw(w, &c); break;
        case APP_ABOUT:    app_about_draw(w, &c); break;
        case APP_AV:       app_av_draw(w, &c); break;
        case APP_EDIT:     app_edit_draw(w, &c); break;
        case APP_SYSMON:   app_sysmon_draw(w, &c); break;
        case APP_IMGVIEW:  app_imgview_draw(w, &c); break;
        case APP_MUSIC:    app_music_draw(w, &c); break;
        case APP_SNAKE:    app_snake_draw(w, &c); break;
        case APP_KALEIDOSCOPE: app_kaleido_draw(w, &c); break;
        case APP_GOL:    app_gol_draw(w, &c); break;
        case APP_SLIDE: app_slide_draw(w, &c); break;
        }
        if (a < 255)
            blend_rect(w->r.x, w->r.y, w->r.w, w->r.h, rgb(0, 0, 0), (u8)(255 - a));
        if (g_focus_mode && w != g_focus)
            blend_rect(w->r.x, w->r.y, w->r.w, w->r.h, rgb(8, 10, 16), 150);
        if (w->anim != 3) w->r = saved_r;
    }

    draw_ctx_menu();
    draw_start_menu();
    draw_taskbar();

    if (snap_on) {
        round_rect_blend(snap_r.x, snap_r.y, snap_r.w, snap_r.h, 10, g_accent, 110);
        round_rect(snap_r.x, snap_r.y, snap_r.w, snap_r.h, 10, mixc(g_accent, rgb(255, 255, 255), 70));
    }
    if (qs_open) draw_quick_settings();
    if (palette_open) draw_command_palette();
    draw_glance();
    draw_toasts();
    if (g_nightlight) blend_rect(0, 0, SW, SH, rgb(255, 150, 40), 26);

    static const char *cursor[] = {
        "X...........",
        "XX..........",
        "XoX.........",
        "XooX........",
        "XoooX.......",
        "XooooX......",
        "XoooooX.....",
        "XooooooX....",
        "XoooooooX...",
        "XooooooooX..",
        "XooooXXXXX..",
        "XooXooX.....",
        "XoX.XooX....",
        "XX..XooX....",
        "X....XooX...",
        ".....XooX...",
        "......XX....",
    };
    for (int j = 0; j < 17; j++)
        for (int i = 0; i < 12; i++) {
            char ch = cursor[j][i];
            if (ch == 'X') putpx(ms_x + i + 1, ms_y + j + 1, rgb(0, 0, 0));
        }
    for (int j = 0; j < 17; j++)
        for (int i = 0; i < 12; i++) {
            char ch = cursor[j][i];
            if (ch == 'X') putpx(ms_x + i, ms_y + j, rgb(10, 10, 14));
            else if (ch == 'o') putpx(ms_x + i, ms_y + j, rgb(245, 246, 250));
        }

    if (ss_on) screensaver_draw();
}

void gui_init(void)
{
    theme_apply();
}
