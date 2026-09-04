#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "timer.h"
#include "mm.h"

static int g_sysmon_tab = 0;
static u32 g_sysmon_last_update = 0;
static int g_cpu_history[60] = {0};
static int g_mem_history[60] = {0};
static int g_hist_pos = 0;
static int g_process_sel = 0;

static const char *tabs[] = { "Processes", "Performance", "System", "Network" };
#define SYSMON_TABS 4

typedef struct {
    char name[32];
    int pid;
    int cpu;
    int mem;
    char state[8];
} ProcessInfo;

static ProcessInfo g_processes[] = {
    { "kikosh", 1, 2, 1024, "Running" },
    { "gui", 2, 5, 2048, "Running" },
    { "files", 3, 1, 1536, "Sleeping" },
    { "terminal", 4, 0, 512, "Sleeping" },
    { "calculator", 5, 0, 256, "Sleeping" },
    { "doodle", 6, 0, 1024, "Sleeping" },
    { "antivirus", 7, 3, 2048, "Running" },
    { "editor", 8, 0, 768, "Sleeping" },
    { "system_monitor", 9, 1, 512, "Running" },
    { "idle", 0, 88, 0, "Running" },
};
#define PROC_COUNT (sizeof(g_processes)/sizeof(g_processes[0]))

void app_sysmon_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    if (ev != ME_PRESS) return;

    // Tab bar (ly is relative to content)
    if (ly < 36) {
        int tab_w = 580 / SYSMON_TABS;
        int tab = lx / tab_w;
        if (tab >= 0 && tab < SYSMON_TABS) {
            g_sysmon_tab = tab;
            ui_request_redraw();
        }
        return;
    }

    if (g_sysmon_tab == 0) { // Processes
        int list_y = 40;
        int item_h = 28;
        if (ly >= list_y) {
            int idx = (ly - list_y) / item_h;
            if (idx >= 0 && idx < PROC_COUNT) {
                g_process_sel = idx;
                ui_request_redraw();
            }
        }
    }
}

void app_sysmon_key(Window *w, int key) {
    (void)w;
    if (key == 0xC8) { g_process_sel = (g_process_sel - 1 + PROC_COUNT) % PROC_COUNT; ui_request_redraw(); }
    else if (key == 0xD0) { g_process_sel = (g_process_sel + 1) % PROC_COUNT; ui_request_redraw(); }
}

static void sysmon_update(void) {
    if (g_ticks - g_sysmon_last_update > 50) {
        g_cpu_history[g_hist_pos] = 12 + (rand32() % 60);
        g_mem_history[g_hist_pos] = 35 + (rand32() % 30);
        g_hist_pos = (g_hist_pos + 1) % 60;
        g_sysmon_last_update = g_ticks;
    }
}

void app_sysmon_draw(Window *w, Rect *c) {
    sysmon_update();
    fill_rect(c->x, c->y, c->w, c->h, rgb(18, 20, 28));

    // Tab bar
    int tab_w = c->w / SYSMON_TABS;
    for (int i = 0; i < SYSMON_TABS; i++) {
        Rect tr = { c->x + i * tab_w, c->y, tab_w, 36 };
        int active = (i == g_sysmon_tab);
        int hover = ui_in(tr, ms_x, ms_y);

        u32 bg = active ? mixc(g_accent, rgb(18,20,28), 80) :
                 hover ? mixc(rgb(255,255,255), rgb(18,20,28), 20) : rgb(18,20,28);
        fill_rect(tr.x, tr.y, tr.w, tr.h, bg);
        if (active) fill_rect(tr.x, tr.y + tr.h - 3, tr.w, 3, g_accent);
        vline(tr.x + tr.w - 1, tr.y, tr.h, rgb(38, 42, 54));

        text(tr.x + tr.w/2 - text_w(tabs[i], 1)/2, tr.y + 12, tabs[i], 1,
             active ? rgb(245,248,252) : (hover ? rgb(220,225,235) : rgb(160,168,184)));
    }

    Rect content = { c->x, c->y + 36, c->w, c->h - 36 };

    switch (g_sysmon_tab) {
    case 0: { // Processes
        // Header
        Rect hdr = { content.x, content.y, content.w, 30 };
        fill_rect(hdr.x, hdr.y, hdr.w, hdr.h, rgb(14, 15, 22));
        hline(hdr.x, hdr.y + hdr.h - 1, hdr.w, rgb(45, 49, 62));
        text(hdr.x + 10, hdr.y + 8, "Name", 1, rgb(140, 148, 164));
        text(hdr.x + 200, hdr.y + 8, "PID", 1, rgb(140, 148, 164));
        text(hdr.x + 260, hdr.y + 8, "CPU %", 1, rgb(140, 148, 164));
        text(hdr.x + 330, hdr.y + 8, "Memory", 1, rgb(140, 148, 164));
        text(hdr.x + 430, hdr.y + 8, "State", 1, rgb(140, 148, 164));

        int item_h = 28;
        int start_y = content.y + 30;
        for (int i = 0; i < PROC_COUNT; i++) {
            int y = start_y + i * item_h;
            if (y + item_h > content.y + content.h) break;
            ProcessInfo *p = &g_processes[i];
            int sel = (i == g_process_sel);
            int hover = ui_in((Rect){content.x, y, content.w, item_h}, ms_x, ms_y);

            u32 bg = sel ? mixc(g_accent, rgb(18,20,28), 70) :
                     hover ? mixc(rgb(255,255,255), rgb(18,20,28), 30) : (i % 2 ? rgb(20,22,30) : rgb(18,20,28));
            fill_rect(content.x, y, content.w, item_h, bg);

            text(content.x + 10, y + 7, p->name, 1, sel ? rgb(245,248,252) : rgb(220,225,235));
            char pid_str[16]; utoa_dec(p->pid, pid_str);
            text(content.x + 200, y + 7, pid_str, 1, rgb(180,188,202));
            char cpu_str[16]; utoa_dec(p->cpu, cpu_str); strcat(cpu_str, "%");
            text(content.x + 260, y + 7, cpu_str, 1, p->cpu > 50 ? rgb(255,100,100) : (p->cpu > 20 ? rgb(255,200,80) : rgb(180,188,202)));
            char mem_str[32]; utoa_dec(p->mem / 1024, mem_str); strcat(mem_str, " MB");
            text(content.x + 330, y + 7, mem_str, 1, rgb(180,188,202));
            text(content.x + 430, y + 7, p->state, 1, strcmp(p->state, "Running")==0 ? rgb(100,220,120) : rgb(180,188,202));
        }
        break;
    }
    case 1: { // Performance
        int graph_w = content.w - 40;
        int graph_h = content.h / 2 - 20;
        int graph_x = content.x + 20;
        int graph_y = content.y + 20;

        // CPU Graph
        round_rect(graph_x, graph_y, graph_w, graph_h, 6, rgb(14, 15, 22));
        rect_outline(graph_x, graph_y, graph_w + 1, graph_h + 1, rgb(38, 42, 54));
        text(graph_x + 10, graph_y + 8, "CPU Usage", 1, rgb(180, 200, 255));

        for (int i = 0; i < 60; i++) {
            int idx = (g_hist_pos + i) % 60;
            int val = g_cpu_history[idx];
            int x = graph_x + graph_w - 2 - i * (graph_w / 60);
            int bar_h = (val * (graph_h - 30)) / 100;
            int bar_y = graph_y + graph_h - 10 - bar_h;
            u32 color = val > 80 ? rgb(255,80,80) : val > 50 ? rgb(255,200,80) : rgb(100,220,120);
            fill_rect(x, bar_y, graph_w / 60 - 1, bar_h, color);
        }

        // Memory Graph
        int mem_y = graph_y + graph_h + 20;
        round_rect(graph_x, mem_y, graph_w, graph_h, 6, rgb(14, 15, 22));
        rect_outline(graph_x, mem_y, graph_w + 1, graph_h + 1, rgb(38, 42, 54));
        text(graph_x + 10, mem_y + 8, "Memory Usage", 1, rgb(255, 180, 100));

        for (int i = 0; i < 60; i++) {
            int idx = (g_hist_pos + i) % 60;
            int val = g_mem_history[idx];
            int x = graph_x + graph_w - 2 - i * (graph_w / 60);
            int bar_h = (val * (graph_h - 30)) / 100;
            int bar_y = mem_y + graph_h - 10 - bar_h;
            u32 color = val > 80 ? rgb(255,80,80) : val > 50 ? rgb(255,200,80) : rgb(100,220,120);
            fill_rect(x, bar_y, graph_w / 60 - 1, bar_h, color);
        }

        // Stats
        int stats_y = mem_y + graph_h + 20;
        u32 used = heap_used() / 1024;
        u32 total = heap_total() / 1024;
        char stat[64];
        strcpy(stat, "Heap: "); utoa_dec(used, stat+6); strcat(stat, " MB / "); utoa_dec(total, stat+strlen(stat)); strcat(stat, " MB");
        text(graph_x + 10, stats_y, stat, 1, rgb(200, 210, 220));
        u32 uptime = g_ticks / 100;
        strcpy(stat, "Uptime: "); utoa_dec(uptime / 3600, stat+8); strcat(stat, "h "); utoa_dec((uptime/60)%60, stat+strlen(stat)); strcat(stat, "m "); utoa_dec(uptime%60, stat+strlen(stat)); strcat(stat, "s");
        text(graph_x + 10, stats_y + 20, stat, 1, rgb(180, 188, 202));
        break;
    }
    case 2: { // System Info
        int y = content.y + 20;
        text(content.x + 20, y, "System Information", 2, g_accent); y += 40;
        char info[128];
        strcpy(info, "OS: KiKOS.11 'Aurora'"); text(content.x + 20, y, info, 1, rgb(220,225,235)); y += 22;
        strcpy(info, "Version: 2026.08 Build"); text(content.x + 20, y, info, 1, rgb(220,225,235)); y += 22;
        strcpy(info, "Kernel: kikos-1.0-i686"); text(content.x + 20, y, info, 1, rgb(220,225,235)); y += 22;
        u32 uptime = g_ticks / 100;
        strcpy(info, "Uptime: "); utoa_dec(uptime / 3600, info+8); strcat(info, "h "); utoa_dec((uptime/60)%60, info+strlen(info)); strcat(info, "m "); utoa_dec(uptime%60, info+strlen(info)); strcat(info, "s");
        text(content.x + 20, y, info, 1, rgb(220,225,235)); y += 22;
        u32 used = heap_used() / 1024; u32 total = heap_total() / 1024;
        strcpy(info, "Memory: "); utoa_dec(used, info+8); strcat(info, " MB / "); utoa_dec(total, info+strlen(info)); strcat(info, " MB");
        text(content.x + 20, y, info, 1, rgb(220,225,235)); y += 22;
        u32 used_pct = used * 100 / total;
        strcpy(info, "Usage: "); utoa_dec(used_pct, info+7); strcat(info, "%");
        text(content.x + 20, y, info, 1, used_pct > 80 ? rgb(255,100,100) : used_pct > 50 ? rgb(255,200,80) : rgb(100,220,120)); y += 30;
        text(content.x + 20, y, "CPU: i686-compatible (QEMU)", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "Display: 1024x768x32 (VBE)", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "Boot: Custom 2-stage MBR -> VBE", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "Filesystem: Virtual (VFS)", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "Desktop: KiWM (custom)", 1, rgb(220,225,235)); y += 22;
        break;
    }
    case 3: { // Network (fake)
        int y = content.y + 20;
        text(content.x + 20, y, "Network", 2, g_accent); y += 40;
        text(content.x + 20, y, "Interface: eth0 (virtio-net)", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "IP: 10.0.2.15 / 24", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "Gateway: 10.0.2.2", 1, rgb(220,225,235)); y += 22;
        text(content.x + 20, y, "DNS: 1.1.1.1, 8.8.8.8", 1, rgb(220,225,235)); y += 30;
        text(content.x + 20, y, "RX: 2.4 MB  TX: 1.1 MB", 1, rgb(180,188,202)); y += 22;
        text(content.x + 20, y, "Status: Connected", 1, rgb(100,220,120)); y += 30;
        text(content.x + 20, y, "Firewall: Active (default deny)", 1, rgb(180,188,202));
        break;
    }
    }
}