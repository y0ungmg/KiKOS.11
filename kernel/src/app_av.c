#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "timer.h"

#define AV_IDLE      0
#define AV_SCANNING  1
#define AV_CLEAN     2
#define AV_THREATS   3

#define WHITE rgb(240, 244, 250)

static int av_state = AV_IDLE;
static u32 av_scan_start = 0;
static int av_file_idx = 0;
static int av_scroll = 0;

static const char *av_files[] = {
    "/boot/stage1.bin",
    "/boot/stage2.bin",
    "/kernel/entry.o",
    "/kernel/isr.o",
    "/kernel/font.c",
    "/kernel/gfx.c",
    "/kernel/gui.c",
    "/kernel/icons.c",
    "/kernel/idt.c",
    "/kernel/kmain.c",
    "/kernel/lib.c",
    "/kernel/login.c",
    "/kernel/mm.c",
    "/kernel/ps2kbd.c",
    "/kernel/ps2mouse.c",
    "/kernel/rtc.c",
    "/kernel/timer.c",
    "/kernel/power.c",
    "/kernel/fs.c",
    "/kernel/app_term.c",
    "/kernel/app_calc.c",
    "/kernel/app_files.c",
    "/kernel/app_doodle.c",
    "/kernel/app_settings.c",
    "/kernel/app_about.c",
    "/kernel/app_av.c",
    "/drivers/keyboard.drv",
    "/drivers/mouse.drv",
    "/drivers/display.drv",
    "/drivers/rtc.drv",
    "/system/config.sys",
    "/system/boot.ini",
    "/users/kikos/.profile",
    "/users/kikos/desktop.ini",
    "/tmp/cache_0x4A2F.tmp",
    "/tmp/session.lock",
};
#define N_AV_FILES (sizeof(av_files) / sizeof(av_files[0]))

#define N_LOG 12
static char log_lines[N_LOG][48];
static int log_count = 0;

static int threats_found = 0;
static char threat_names[4][48];
static int n_threats = 0;

static void log_add(const char *line)
{
    if (log_count >= N_LOG) {
        for (int i = 0; i < N_LOG - 1; i++)
            strncpy(log_lines[i], log_lines[i + 1], 47);
        log_count = N_LOG - 1;
    }
    strncpy(log_lines[log_count], line, 47);
    log_lines[log_count][47] = 0;
    log_count++;
}

static void log_clear(void) { log_count = 0; }

static void start_scan(void)
{
    av_state = AV_SCANNING;
    av_scan_start = g_ticks;
    av_file_idx = 0;
    av_scroll = 0;
    log_clear();
    threats_found = 0;
    n_threats = 0;
    log_add("KiKOS Antivirus v1.0");
    log_add("Initializing scan engine...");
}

static void simulate_threat(int idx)
{
    static const char *fake_threats[] = {
        "Trojan.KiKOS.memdump",
        "Worm.Suspicious.kernel_mod",
        "Adware.Temp.cache_leak",
        "Spy.ReadKey.logger_hint",
    };
    if (n_threats < 4) {
        strncpy(threat_names[n_threats], fake_threats[idx % 4], 47);
        n_threats++;
    }
    threats_found = 1;
}

void app_av_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w;
    if (ev != ME_RELEASE) return;

    Rect c = win_content(w);
    int bw = 160, bh = 36;
    int bx = c.w / 2 - bw / 2;
    int by = c.h - 50;
    Rect scan_btn = { bx, by, bw, bh };

    if (ui_in(scan_btn, lx, ly)) {
        if (av_state == AV_IDLE || av_state == AV_CLEAN || av_state == AV_THREATS)
            start_scan();
    }
}

void app_av_draw(Window *w, Rect *c)
{
    fill_rect(c->x, c->y, c->w, c->h, rgb(14, 16, 24));

    int cx_s = c->x + c->w / 2;

    u32 shield_c = av_state == AV_CLEAN ? rgb(60, 200, 120) :
                    av_state == AV_THREATS ? rgb(220, 70, 70) :
                    av_state == AV_SCANNING ? g_accent_a : rgb(100, 110, 130);

    int shield_y = c->y + 12;
    int shield_s = 52;
    int scx = cx_s, scy = shield_y + shield_s / 2;
    for (int j = 0; j < shield_s; j++) {
        int w_row;
        if (j < shield_s / 2)
            w_row = (shield_s / 2 - 2) * j / (shield_s / 2);
        else
            w_row = (shield_s / 2 - 2) * (shield_s - j) / (shield_s / 2);
        if (w_row < 1) w_row = 1;
        hline(scx - w_row, shield_y + j, w_row * 2, mixc(shield_c, rgb(0, 0, 0), 40));
    }
    circle_ring(scx, scy, shield_s / 2 - 1, mixc(shield_c, WHITE, 60));

    if (av_state == AV_SCANNING) {
        u32 pulse = (g_ticks / 2) % 40;
        u8 ga = (u8)(pulse < 20 ? pulse * 6 : (40 - pulse) * 6);
        if (ga > 100) ga = 100;
        for (int r = shield_s / 2 + 4; r < shield_s / 2 + 14; r++) {
            u32 a2 = ga * (u32)(14 - (r - shield_s / 2 - 4)) / 10;
            if (a2 > 255) a2 = 255;
            circle_ring(scx, scy, r, mixc(shield_c, rgb(0, 0, 0), (u8)(255 - a2)));
        }
    }

    if (av_state == AV_CLEAN) {
        draw_line(scx - 8, scy + 1, scx - 2, scy + 8, rgb(240, 245, 250));
        draw_line(scx - 2, scy + 8, scx + 9, scy - 7, rgb(240, 245, 250));
    } else if (av_state == AV_THREATS) {
        draw_line(scx - 6, scy - 6, scx + 6, scy + 6, rgb(240, 245, 250));
        draw_line(scx + 6, scy - 6, scx - 6, scy + 6, rgb(240, 245, 250));
    } else if (av_state == AV_IDLE) {
        text(scx - 5, scy - 5, "?", 2, rgb(240, 245, 250));
    }

    const char *title = "KiKOS Antivirus";
    text(cx_s - text_w(title, 2) / 2, shield_y + shield_s + 10, title, 2, rgb(240, 244, 250));

    const char *status =
        av_state == AV_IDLE ? "Ready to scan" :
        av_state == AV_SCANNING ? "Scanning system files..." :
        av_state == AV_CLEAN ? "System is clean!" :
        "Threats detected!";
    u32 st_c = av_state == AV_CLEAN ? rgb(60, 200, 120) :
               av_state == AV_THREATS ? rgb(220, 70, 70) :
               av_state == AV_SCANNING ? g_accent_a : rgb(140, 148, 168);
    text(cx_s - text_w(status, 1) / 2, shield_y + shield_s + 32, status, 1, st_c);

    int bar_x = c->x + 30;
    int bar_y = shield_y + shield_s + 52;
    int bar_w = c->w - 60;
    int bar_h = 14;
    round_rect(bar_x, bar_y, bar_w, bar_h, 7, rgb(22, 24, 34));
    rect_outline(bar_x, bar_y, bar_w + 1, bar_h + 1, rgb(40, 44, 58));

    int progress = 0;
    if (av_state == AV_SCANNING) {
        u32 elapsed = g_ticks - av_scan_start;
        progress = (int)(elapsed * 100 / 180);
        if (progress > 100) progress = 100;
    } else if (av_state == AV_CLEAN || av_state == AV_THREATS) {
        progress = 100;
    }

    if (progress > 0) {
        int pw = (bar_w - 4) * progress / 100;
        u32 pc = av_state == AV_THREATS ? rgb(200, 60, 60) : g_accent_a;
        for (int j = 0; j < bar_h - 4; j++) {
            u32 c2 = mixc(pc, rgb(0, 0, 0), (u8)(j * 30 / (bar_h - 4)));
            hline(bar_x + 2, bar_y + 2 + j, pw, c2);
        }
    }

    char pct_buf[8];
    utoa_dec((u32)progress, pct_buf);
    strcat(pct_buf, "%");
    text(bar_x + bar_w / 2 - text_w(pct_buf, 1) / 2, bar_y + 3, pct_buf, 1,
         progress > 0 ? rgb(240, 244, 250) : rgb(80, 86, 100));

    int log_x = c->x + 16;
    int log_y = bar_y + bar_h + 12;
    int log_w = c->w - 32;
    int log_h = c->h - (log_y - c->y) - 56;

    round_rect(log_x, log_y, log_w, log_h, 6, rgb(10, 11, 18));
    rect_outline(log_x, log_y, log_w + 1, log_h + 1, rgb(32, 36, 48));

    int ly = log_y + 6;
    int max_lines = log_h / 12;
    int start_i = log_count > max_lines ? log_count - max_lines : 0;
    for (int i = start_i; i < log_count && ly < log_y + log_h - 4; i++) {
        u32 lc = rgb(120, 130, 150);
        if (log_lines[i][0] == '!' || log_lines[i][0] == 'X')
            lc = rgb(220, 70, 70);
        else if (log_lines[i][0] == '>')
            lc = rgb(60, 200, 120);
        text(log_x + 8, ly, log_lines[i], 1, lc);
        ly += 12;
    }

    if (av_state == AV_THREATS && n_threats > 0) {
        int tly = log_y + log_h + 6;
        text(log_x + 4, tly, "Threats:", 1, rgb(220, 70, 70));
        for (int i = 0; i < n_threats && i < 3; i++) {
            text(log_x + 70, tly, threat_names[i], 1, rgb(200, 160, 80));
            tly += 12;
        }
    }

    Rect scan_btn = { cx_s - 80, c->y + c->h - 50, 160, 36 };
    int scanning = av_state == AV_SCANNING;
    u32 btn_bg = scanning ? rgb(30, 34, 44) :
                 ui_in(scan_btn, ms_x, ms_y) ? rgb(55, 150, 200) : rgb(35, 100, 170);
    if (ms_btn_l && ui_in(scan_btn, ms_x, ms_y) && !scanning)
        btn_bg = mixc(btn_bg, rgb(255, 255, 255), 30);
    round_rect(scan_btn.x, scan_btn.y, scan_btn.w, scan_btn.h, 10, btn_bg);
    if (!scanning) {
        blend_rect(scan_btn.x + 4, scan_btn.y + 1, scan_btn.w - 8, 2,
                   rgb(255, 255, 255), 30);
    }
    const char *btn_txt = scanning ? "Scanning..." :
                          (av_state == AV_CLEAN || av_state == AV_THREATS) ? "Rescan" : "Scan Now";
    text(cx_s - text_w(btn_txt, 2) / 2, scan_btn.y + 10, btn_txt, 2, rgb(240, 244, 250));
}

void app_av_tick(void)
{
    if (av_state != AV_SCANNING) return;

    u32 elapsed = g_ticks - av_scan_start;
    int new_idx = (int)(elapsed * (int)N_AV_FILES / 180);
    if (new_idx > (int)N_AV_FILES) new_idx = (int)N_AV_FILES;

    while (av_file_idx < new_idx && av_file_idx < (int)N_AV_FILES) {
        char line[56];
        strcpy(line, "> ");
        strcat(line, av_files[av_file_idx]);
        log_add(line);

        if (av_file_idx == 14 || av_file_idx == 26) {
            char tline[56];
            strcpy(tline, "! ALERT: ");
            strcat(tline, av_files[av_file_idx]);
            log_add(tline);
            simulate_threat(av_file_idx);
        }

        av_file_idx++;
    }

    if (av_file_idx >= (int)N_AV_FILES) {
        if (threats_found) {
            av_state = AV_THREATS;
            char msg[48];
            strcpy(msg, "X ");
            utoa_dec((u32)n_threats, msg + 2);
            strcat(msg, " threat(s) found!");
            log_add(msg);
        } else {
            av_state = AV_CLEAN;
            log_add("> Scan complete. System is clean.");
        }
        log_add("> Files scanned: ");
        utoa_dec((u32)N_AV_FILES, log_lines[log_count - 1] + 17);
    }
}
