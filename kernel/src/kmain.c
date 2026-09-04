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

static void splash(void)
{
    wall_blit();

    int cx = SW / 2, cy = SH / 2 - 30;

    for (int j = -30; j <= 114; j++) {
        int dist = j < 0 ? -j : (j > 84 ? j - 84 : 0);
        int alpha = dist > 0 ? 40 - dist : 40;
        if (alpha > 0) {
            u32 glow_c = mixc(g_accent_a, g_accent_b, (u8)(128));
            blend_rect(cx - 42 - 20, cy - 42 + j, 84 + 40, 1, glow_c, (u8)alpha);
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

    int barw = 260;
    for (int t = 0; t < 90; t++) {
        int p = t * barw / 90;
        fill_rect(cx - barw / 2 - 1, cy + 140, barw + 2, 8, rgb(10, 12, 20));
        for (int j = 0; j < 6; j++)
            hline(cx - barw / 2, cy + 141 + j, p > barw ? barw : p,
                  mixc(g_accent_a, g_accent_b, (u8)(j * 42)));
        arc(cx + barw / 2 + 26, cy + 144, 7, (int)(g_ticks * 14) % 360,
            (int)((g_ticks * 14) % 360) + 300, rgb(235, 240, 248));

        u32 end = g_ticks + 1;
        while ((i32)(g_ticks - end) < 0) __asm__ volatile("hlt");
    }

    sleep_ticks(25);
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