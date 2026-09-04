#include "login.h"
#include "gfx.h"
#include "lib.h"
#include "timer.h"
#include "rtc.h"
#include "ps2kbd.h"
#include "ps2mouse.h"
#include "icons.h"
#include "gui.h"
#include "power.h"
#include "pcspk.h"

#define N_PARTICLES 120
#define N_ORBS      5
#define MAX_PASS    32

typedef struct {
    int x, y;
    int vx, vy;
    int size;
    u8  bright;
    u8  cr, cg, cb;
} Particle;

typedef struct {
    int x, y, rad;
    int vx, vy;
    u8  cr, cg, cb;
    u8  alpha;
} Orb;

static Particle parts[N_PARTICLES];
static Orb orbs[N_ORBS];

static char pass[MAX_PASS];
static int  pass_len = 0;
static int  cursor_blink = 0;
static int  fade_alpha = 0;
static int  fading_in = 0;
static int  login_done = 0;

static int prev_mb = 0;
static int mb_just_released = 0;
static int mb_down_x, mb_down_y;

static void track_mouse(void)
{
    mb_just_released = (prev_mb && !ms_btn_l);
    if (ms_btn_l && !prev_mb) {
        mb_down_x = ms_x;
        mb_down_y = ms_y;
    }
    prev_mb = ms_btn_l;
}

static int login_clicked(int x, int y, int w, int h)
{
    Rect r = { x, y, w, h };
    return mb_just_released && ui_in(r, ms_x, ms_y) && ui_in(r, mb_down_x, mb_down_y);
}

static void spawn_particle(Particle *p)
{
    p->x = (int)(rand32() % (u32)SW);
    p->y = (int)(rand32() % (u32)SH);
    p->vx = (int)(rand32() % 7) - 3;
    p->vy = (int)(rand32() % 5) - 4;
    p->size = 1 + (int)(rand32() % 3);
    p->bright = (u8)(30 + (int)(rand32() % 90));
    u8 pick = (u8)(rand32() % 3);
    if (pick == 0) { p->cr = 100; p->cg = 180; p->cb = 255; }
    else if (pick == 1) { p->cr = 160; p->cg = 120; p->cb = 255; }
    else { p->cr = 80; p->cg = 220; p->cb = 200; }
}

static void spawn_orb(Orb *o)
{
    o->x = (int)(rand32() % (u32)(SW / 2)) + SW / 4;
    o->y = (int)(rand32() % (u32)(SH / 2)) + SH / 4;
    o->rad = 80 + (int)(rand32() % 120);
    o->vx = (int)(rand32() % 5) - 2;
    o->vy = (int)(rand32() % 5) - 2;
    u8 pick = (u8)(rand32() % 3);
    if (pick == 0) { o->cr = 50; o->cg = 180; o->cb = 230; }
    else if (pick == 1) { o->cr = 140; o->cg = 80; o->cb = 220; }
    else { o->cr = 60; o->cg = 220; o->cb = 180; }
    o->alpha = 20 + (u8)(rand32() % 30);
}

int login_init(void)
{
    pass_len = 0;
    pass[0] = 0;
    cursor_blink = 0;
    fade_alpha = 0;
    fading_in = 0;
    login_done = 0;
    prev_mb = 0;

    rand_seed(g_ticks ^ 0xDEADBEEF);

    for (int i = 0; i < N_PARTICLES; i++)
        spawn_particle(&parts[i]);
    for (int i = 0; i < N_ORBS; i++)
        spawn_orb(&orbs[i]);

    return 0;
}

static void update_particles(void)
{
    for (int i = 0; i < N_PARTICLES; i++) {
        Particle *p = &parts[i];
        p->x += p->vx;
        p->y += p->vy;
        if (p->x < -10) p->x = SW + 5;
        if (p->x > SW + 10) p->x = -5;
        if (p->y < -10) p->y = SH + 5;
        if (p->y > SH + 10) p->y = -5;
    }
    for (int i = 0; i < N_ORBS; i++) {
        Orb *o = &orbs[i];
        o->x += o->vx;
        o->y += o->vy;
        if (o->x < -o->rad) o->x = SW + o->rad / 2;
        if (o->x > SW + o->rad) o->x = -o->rad / 2;
        if (o->y < -o->rad) o->y = SH + o->rad / 2;
        if (o->y > SH + o->rad) o->y = -o->rad / 2;
    }
}

static void draw_bg(void)
{
    int w = SW < 1024 ? SW : 1024;
    int h = SH < 768 ? SH : 768;

    for (int j = 0; j < h; j++) {
        u32 *row = fb + (u32)j * PITCH;
        for (int i = 0; i < w; i++) {
            u32 t = ((u32)(i + j) * 510u) / (u32)(w + h);
            u32 c = mixc(rgb(6, 10, 28), rgb(18, 24, 52), (u8)(t > 255 ? 255 : t));
            if (t > 255) c = mixc(c, rgb(40, 18, 68), (u8)(t - 255));
            row[i] = c;
        }
    }

    for (int i = 0; i < N_ORBS; i++) {
        Orb *o = &orbs[i];
        u32 oc = rgb(o->cr, o->cg, o->cb);
        int r2 = o->rad * o->rad;
        for (int j = o->y - o->rad; j <= o->y + o->rad; j++) {
            if (j < 0 || j >= SH) continue;
            for (int ii = o->x - o->rad; ii <= o->x + o->rad; ii++) {
                if (ii < 0 || ii >= SW) continue;
                int dx = ii - o->x, dy = j - o->y;
                u32 d2 = (u32)(dx * dx + dy * dy);
                if (d2 > (u32)r2) continue;
                u32 fall = (255 - d2 * 255 / (u32)r2) * (u32)o->alpha / 255;
                if (fall < 1) continue;
                if (fall > 255) fall = 255;
                u32 idx = (u32)j * PITCH + (u32)ii;
                fb[idx] = mixc(fb[idx], oc, (u8)fall);
            }
        }
    }

    for (int i = 0; i < N_PARTICLES; i++) {
        Particle *p = &parts[i];
        u32 pc = mixc(rgb(0, 0, 0), rgb(p->cr, p->cg, p->cb), p->bright);
        if (p->size == 1) {
            putpx(p->x, p->y, pc);
        } else if (p->size == 2) {
            putpx(p->x, p->y, pc);
            putpx(p->x + 1, p->y, pc);
            putpx(p->x, p->y + 1, pc);
            putpx(p->x + 1, p->y + 1, pc);
        } else {
            u32 pc2 = mixc(rgb(0, 0, 0), rgb(p->cr, p->cg, p->cb), p->bright / 2);
            putpx(p->x - 1, p->y, pc2);
            putpx(p->x + 1, p->y, pc2);
            putpx(p->x, p->y - 1, pc2);
            putpx(p->x, p->y + 1, pc2);
            putpx(p->x, p->y, pc);
        }
    }
}

static void draw_glass_card(int cx, int cy, int cw, int ch)
{
    blend_rect(cx + 4, cy + 6, cw, ch, rgb(0, 0, 0), 60);
    round_rect(cx, cy, cw, ch, 18, rgb(14, 16, 30));
    blend_rect(cx + 1, cy + 1, cw - 2, ch / 3, rgb(80, 100, 160), 10);
    rect_outline(cx, cy, cw + 1, ch + 1, rgb(50, 55, 80));
    hline(cx + 20, cy + 1, cw - 40, rgb(90, 100, 160));

    u32 br = (g_ticks / 2) % 120;
    u8 ga = (u8)(br < 60 ? br * 2 : (120 - br) * 2);
    if (ga > 70) ga = 70;
    blend_rect(cx - 2, cy - 2, cw + 4, ch + 4, rgb(90, 150, 220), ga);
}

static void draw_avatar(int cx, int cy, int r)
{
    circle_fill(cx, cy, r, rgb(20, 24, 50));

    u32 pulse = (g_ticks / 3) % 60;
    u8 ga = (u8)(pulse < 30 ? pulse * 3 : (60 - pulse) * 3);
    if (ga > 80) ga = 80;
    int gr = r + 6;
    int gr2 = gr * gr;
    int r2 = r * r;
    for (int j = cy - gr; j <= cy + gr; j++) {
        if (j < 0 || j >= SH) continue;
        for (int i = cx - gr; i <= cx + gr; i++) {
            if (i < 0 || i >= SW) continue;
            int dx = i - cx, dy = j - cy;
            u32 d2 = (u32)(dx * dx + dy * dy);
            if (d2 > (u32)gr2 || d2 < (u32)r2) continue;
            u32 fall = (255 - d2 * 255 / (u32)gr2) * (u32)ga / 255;
            if (fall < 1) continue;
            if (fall > 255) fall = 255;
            fb[(u32)j * PITCH + (u32)i] =
                mixc(fb[(u32)j * PITCH + (u32)i], rgb(60, 180, 220), (u8)fall);
        }
    }

    circle_ring(cx, cy, r, rgb(60, 160, 200));
    circle_ring(cx, cy, r - 1, mixc(rgb(60, 160, 200), rgb(0, 0, 0), 100));
    text(cx - 12, cy - 12, "K", 3, rgb(240, 245, 255));
}

static void draw_field(int x, int y, int w, int h)
{
    u32 bg = rgb(22, 26, 48);
    u32 border = rgb(60, 160, 210);
    round_rect(x, y, w, h, 10, bg);
    rect_outline(x, y, w + 1, h + 1, border);

    blend_rect(x - 1, y - 1, w + 3, h + 3, rgb(60, 160, 210), 15);

    int py = y + h / 2 - 5;
    for (int i = 0; i < pass_len; i++)
        circle_fill(x + 14 + i * 14, py + 5, 4, rgb(200, 210, 230));

    if (cursor_blink < 30) {
        int cxp = x + 14 + pass_len * 14;
        fill_rect(cxp, y + 6, 2, h - 12, rgb(200, 220, 255));
    }
}

static void draw_sign_btn(int x, int y, int w, int h, const char *label)
{
    int hover = ui_in((Rect){x, y, w, h}, ms_x, ms_y);
    u32 bg = hover ? rgb(55, 150, 200) : rgb(35, 100, 170);
    if (ms_btn_l && hover) bg = mixc(bg, rgb(255, 255, 255), 30);
    round_rect(x, y, w, h, 12, bg);
    blend_rect(x + 4, y + 1, w - 8, 2, rgb(255, 255, 255), 40);
    blend_rect(x + 4, y + 3, w - 8, 1, rgb(255, 255, 255), 15);
    text(x + w / 2 - text_w(label, 2) / 2,
         y + h / 2 - 8, label, 2, rgb(245, 248, 255));
}

int login_frame(void)
{
    if (login_done) return 1;

    track_mouse();
    cursor_blink = (cursor_blink + 1) % 60;

    rtc_poll();
    update_particles();
    draw_bg();

    int cx = SW / 2;
    int cy = SH / 2;

    char time_buf[24];
    format_time(time_buf, sizeof time_buf);
    char date_buf[24];
    format_date(date_buf, sizeof date_buf);

    int clock_y = cy - 210;
    int tw = text_w(time_buf, 5);
    text(cx - tw / 2, clock_y, time_buf, 5, rgb(240, 244, 252));

    int dw = text_w(date_buf, 2);
    text(cx - dw / 2, clock_y + 52, date_buf, 2,
         mixc(rgb(200, 210, 240), rgb(0, 0, 0), 80));

    int card_w = 340;
    int card_h = 320;
    int card_x = cx - card_w / 2;
    int card_y = cy - card_h / 2 + 20;

    draw_glass_card(card_x, card_y, card_w, card_h);

    draw_avatar(cx, card_y + 55, 30);

    const char *uname = "kikos";
    int uw = text_w(uname, 2);
    text(cx - uw / 2, card_y + 97, uname, 2, rgb(230, 235, 245));

    const char *host = "KiKOS.11 PC";
    int hw = text_w(host, 1);
    text(cx - hw / 2, card_y + 117, host, 1,
         mixc(rgb(160, 170, 200), rgb(0, 0, 0), 60));

    const char *tag = "your os, your way";
    int tgw = text_w(tag, 1);
    text(cx - tgw / 2, card_y + 135, tag, 1, mixc(rgb(120, 160, 220), rgb(0, 0, 0), 40));

    text(card_x + 24, card_y + 146, "Password:", 1, rgb(140, 150, 175));
    draw_field(card_x + 24, card_y + 162, card_w - 48, 36);

    Rect sbtn = { card_x + 24, card_y + card_h - 60, card_w - 48, 42 };
    draw_sign_btn(sbtn.x, sbtn.y, sbtn.w, sbtn.h, "Sign In");
    if (login_clicked(sbtn.x, sbtn.y, sbtn.w, sbtn.h)) {
        fading_in = 1;
        beep(440, 60);
    }

    int hy = card_y + card_h + 16;
    const char *hint = "Press Enter or click Sign In";
    int hintw = text_w(hint, 1);
    text(cx - hintw / 2, hy, hint, 1, mixc(rgb(120, 130, 170), rgb(0, 0, 0), 50));

    Rect pwr = { SW - 50, SH - 54, 36, 36 };
    circle_fill(pwr.x + 18, pwr.y + 18, 18,
                ui_in(pwr, ms_x, ms_y) ? rgb(60, 30, 35) : rgb(30, 18, 22));
    icon_draw(ICON_POWER, pwr.x + 10, pwr.y + 10, 16);
    if (login_clicked(pwr.x, pwr.y, pwr.w, pwr.h))
        power_off();

    int key;
    while ((key = kbd_pop()) != -1) {
        if (key == '\n' || key == '\r') {
            fading_in = 1;
            beep(440, 60);
        } else if (key == '\b') {
            if (pass_len > 0) { pass[--pass_len] = 0; }
        } else if (key >= 32 && key < 127 && pass_len < MAX_PASS - 1) {
            pass[pass_len++] = (char)key;
            pass[pass_len] = 0;
        }
        cursor_blink = 0;
    }

    if (fading_in) {
        login_done = 1;
    }

    return login_done;
}
