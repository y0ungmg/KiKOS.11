#include "types.h"
#include "bootinfo.h"
#include "lib.h"
#include "com1.h"
#include "idt.h"
#include "timer.h"
#include "ps2kbd.h"
#include "ps2mouse.h"
#include "rtc.h"
#include "gfx.h"
#include "gui.h"
#include "icons.h"
#include "mm.h"
#include "apps.h"
#include "login.h"
#include "vfs.h"
#include "pcspk.h"

static BootInfo *g_bi;
static char g_cpu_brand[49];
static u32 g_mem_kb = 0;

const char *cpu_brand(void)
{
    if (!g_cpu_brand[0]) return "x86 CPU";
    return g_cpu_brand;
}

u32 total_mem_kb(void)
{
    return g_mem_kb ? g_mem_kb : 640u;
}

void isr_dispatch(u32 n)
{
    switch (n) {
    case 32:
        irq_timer_handler();
        {
            static u32 last_sec = 0xFFFFFFFF;
            if (g_ticks / 100 != last_sec) {
                last_sec = g_ticks / 100;
                g_input_epoch++;
            }
        }
        break;
    case 33:
        irq_kbd_handler();
        g_input_epoch++;
        break;
    case 44:
        irq_mouse_handler();
        g_input_epoch++;
        break;
    default:
        if (n >= 40 && n < 48) irq_eoi(n - 32);
        else if (n >= 32 && n < 40) irq_eoi(n - 32);
        break;
    }
}

static const char *const splash_steps[] = {
    "detecting CPU",
    "probing memory",
    "initializing drivers",
    "starting display",
    "loading desktop"
};
#define SPLASH_STEPS 5

static void splash_draw_bar(int cx, int cy, int barw, int pct, u32 tick)
{
    fill_rect(cx - barw / 2 - 1, cy + 140, barw + 2, 10, rgb(8, 10, 18));
    fill_rect(cx - barw / 2 - 1, cy + 140, barw + 2, 2, rgb(20, 24, 40));
    fill_rect(cx - barw / 2 - 1, cy + 148, barw + 2, 2, rgb(20, 24, 40));
    if (pct > 0) {
        int pw = pct * barw / 100;
        for (int j = 0; j < 8; j++)
            hline(cx - barw / 2, cy + 142 + j, pw,
                  mixc(g_accent_a, g_accent_b, (u8)(j * 32)));
        int shim = (int)(tick * 40) % (barw * 2) - barw;
        for (int j = 0; j < 8; j++)
            hline(cx - barw / 2 + shim, cy + 142 + j,
                  shim + 28 > barw ? barw - shim : 28, rgb(255, 255, 255));
    }
}

static void splash(void)
{
    wall_blit();

    int cx = SW / 2, cy = SH / 2 - 30;

    u32 t0 = uptime_ms();
    int total_ms = 2300;
    int step_ms = total_ms / SPLASH_STEPS;
    int step = 0;

    while (uptime_ms() - t0 < (u32)total_ms) {
        u32 now = uptime_ms() - t0;
        u32 ticks = g_ticks;

        int pct = (int)(now * 100 / total_ms);
        int cur_step = (int)(now / step_ms);
        if (cur_step >= SPLASH_STEPS) cur_step = SPLASH_STEPS - 1;
        if (cur_step != step) {
            step = cur_step;
            beep_click();
        }

        wall_blit();

        /* pulsing glow behind logo */
        int pulse = 26 + (int)((ticks % 40) * 2);
        for (int j = -30; j <= 114; j++) {
            int dist = j < 0 ? -j : (j > 84 ? j - 84 : 0);
            int alpha = dist > 0 ? 44 - dist : 44;
            if (alpha > 0) {
                u32 glow_c = mixc(g_accent_a, g_accent_b, (u8)(128 + (ticks % 60)));
                blend_rect(cx - 42 - pulse, cy - 42 + j, 84 + pulse * 2, 1, glow_c, (u8)alpha);
            }
        }

        for (int j = 0; j < 84; j++) {
            u32 c = mixc(g_accent_a, g_accent_b, (u8)((j * 255) / 84));
            hline(cx - 42, cy - 42 + j, 84, c);
        }

        text(cx - 12, cy - 12, "K", 3, rgb(250, 252, 255));

        const char *nm = "KiKOS.11";
        text(cx - text_w(nm, 4) / 2, cy + 66, nm, 4, rgb(245, 248, 252));

        const char *sub = "'Aurora'  build 2026.08";
        text(cx - text_w(sub, 1) / 2, cy + 106, sub, 1, mixc(g_accent_a, rgb(255, 255, 255), 60));

        splash_draw_bar(cx, cy, 260, pct, ticks);

        /* step label */
        text(cx - text_w(splash_steps[cur_step], 1) / 2, cy + 158,
             splash_steps[cur_step], 1, (cur_step == step) ? rgb(235, 240, 248)
                                                           : mixc(rgb(90, 96, 116), rgb(190, 198, 214), 120));

        /* spinner */
        arc(cx + 156, cy + 144, 7, (int)(ticks * 14) % 360,
            (int)((ticks * 14) % 360) + 300, rgb(235, 240, 248));

        int wait = 8;
        u32 end = g_ticks + 1;
        while ((i32)(g_ticks - end) < 0 && wait--) __asm__ volatile("hlt");
    }

    sleep_ticks(15);
}

void kmain(BootInfo *bi)
{
    g_bi = bi;

    dbg_init();
    pcspk_init();
    dbg("[kikos] kernel alive\n");

    u32 regs[12];
    __asm__ volatile(
        "movl $0x80000002, %%eax\n\t"
        "cpuid"
        : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        :
        : "memory");
    __asm__ volatile(
        "movl $0x80000003, %%eax\n\t"
        "cpuid"
        : "=a"(regs[4]), "=b"(regs[5]), "=c"(regs[6]), "=d"(regs[7])
        :
        : "memory");
    __asm__ volatile(
        "movl $0x80000004, %%eax\n\t"
        "cpuid"
        : "=a"(regs[8]), "=b"(regs[9]), "=c"(regs[10]), "=d"(regs[11])
        :
        : "memory");
    memcpy(g_cpu_brand, regs, 48);
    g_cpu_brand[48] = 0;

    u32 brand_ok = g_cpu_brand[0] != 0;
    for (int i = 0; i < 48 && brand_ok == 0; i++)
        if (g_cpu_brand[i] >= 32 && g_cpu_brand[i] < 127) { brand_ok = 1; break; }
    if (!brand_ok || g_cpu_brand[0] == (char)0xFF)
        strcpy(g_cpu_brand, "i686-compatible CPU");

    g_mem_kb = bi->ext_mem_kb ? bi->ext_mem_kb : 130048;

    gfx_init(bi);
    dbg("[kikos] framebuffer up\n");
    dbg("[kikos] drivers ready\n");

    splash();
    dbg("[kikos] desktop\n");

    login_init();
    int phase = 0;
    u32 fade_start = 0;
    u32 FADE_MS = 800;

    u32 last_epoch = g_input_epoch;
    for (;;) {
        __asm__ volatile("hlt");
        if (g_input_epoch != last_epoch) {
            last_epoch = g_input_epoch;

            if (phase == 0) {
                if (login_frame()) {
                    dbg("[kikos] login->desktop\n");
                    phase = 1;
                    g_gui_active = 1;
                    fade_start = uptime_ms();
                    theme_apply();
                    beep_ok();
                }
            } else {
                int key;
                while ((key = kbd_pop()) != -1) {
                    if (gui_palette_toggle(key)) continue;
                    if (gui_handle_palette_key(key)) continue;
                    if (gui_handle_fkey(key)) continue;
                    if (gui_handle_key(key)) continue;
                    if (g_focus && g_focus->visible) {
                        switch (g_focus->app) {
                        case APP_TERM: app_term_key(g_focus, key); break;
                        case APP_CALC: app_calc_key(g_focus, key); break;
                        case APP_EDIT: app_edit_key(g_focus, key); break;
                        case APP_SYSMON: app_sysmon_key(g_focus, key); break;
                        case APP_SNAKE: app_snake_key(g_focus, key); break;
                        case APP_KALEIDOSCOPE: app_kaleido_key(g_focus, key); break;
                        case APP_GOL: app_gol_key(g_focus, key); break;
                        case APP_SLIDE: app_slide_key(g_focus, key); break;
                        default: break;
                        }
                    }
                }

                ms_state_dirty = 0;
                app_av_tick();
                gui_frame();

                u32 elapsed = uptime_ms() - fade_start;
                if (elapsed < FADE_MS) {
                    u8 alpha = (u8)(255 - elapsed * 255 / FADE_MS);
                    blend_rect(0, 0, SW, SH, rgb(0, 0, 0), alpha);
                }
            }
        }
    }
}