#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "timer.h"
#include "rtc.h"
#include "gui.h"
#include "icons.h"
#include "power.h"
#include "mm.h"
#include "vfs.h"
#include "firewall.h"
#include "antivirus.h"
#include <stdio.h>

#define TERM_COLS  62
#define TERM_MAXL  200

static char lines[TERM_MAXL][TERM_COLS + 1];
static u32  line_col[TERM_MAXL];
static u32  cur_col = 0xC8CEDA;
static int  count = 0;
static char input[128];
static int  input_len = 0;

static void scroll_up(void)
{
    memmove(lines[0], lines[1], (u32)(TERM_MAXL - 1) * (TERM_COLS + 1));
    memmove(line_col, line_col + 1, (u32)(TERM_MAXL - 1) * sizeof(u32));
}

static void new_line(void)
{
    if (count == TERM_MAXL) {
        scroll_up();
        count--;
    }
    memset(lines[count], 0, TERM_COLS + 1);
    line_col[count] = cur_col;
    count++;
}

static void term_push(const char *s)
{
    for (; *s; s++) {
        if (*s == '\n') {
            new_line();
            continue;
        }
        int col = (int)strlen(lines[count ? count - 1 : 0]);
        int r = count ? count - 1 : 0;
        if (col >= TERM_COLS || (!count)) {
            if (col >= TERM_COLS) {
                new_line();
                r = count - 1;
                col = 0;
            } else if (!count) {
                new_line();
                r = 0;
                col = 0;
            }
        }
        lines[r][col] = *s;
        lines[r][col + 1] = 0;
        line_col[r] = cur_col;
    }
}

static void term_color(u32 c) { cur_col = c; }

void app_term_open(Window *w)
{
    (void)w;
    count = 0;
    input_len = 0;
    input[0] = 0;
    cur_col = rgb(200, 206, 218);
    memset(lines, 0, sizeof(lines));
    term_push("KiKOS shell v1.0 - type 'help'\n");
    term_push("kikos> ");
}

/* ---------------- commands ---------------- */

static const char *LOGO[] = {
    " _  ___ _  ___ _____ ",
    "| |/ / | |/ _ \\_   _|",
    "| ' <| |_| (_) || |  ",
    "|_|\\_\\\\___|\\___/ |_|  ",
};

static void cmd_help(void)
{
    term_color(mixc(g_accent, rgb(255, 255, 255), 40));
    term_push("commands:\n");
    term_color(rgb(200, 206, 218));
    term_push(
        "  help          this list\n"
        "  kikofetch     system summary\n"
        "  about         what is KiKOS\n"
        "  ver           version info\n"
        "  echo TEXT     print text\n"
        "  ls [DIR]      list files\n"
        "  cat FILE      show a file\n"
        "  mkdir DIR     create directory\n"
        "  rm FILE/DIR   remove file or directory\n"
        "  cp SRC DST    copy file\n"
        "  mv SRC DST    move/rename file\n"
        "  find [DIR]    find files\n"
        "  grep PAT FILE search text\n"
        "  ps            list processes\n"
        "  kill PID      kill process\n"
        "  df            disk usage\n"
        "  du [DIR]      directory size\n"
        "  top           system monitor\n"
        "  ping HOST     ping host\n"
        "  curl URL      HTTP request\n"
        "  wget URL      download file\n"
        "  ssh HOST      SSH connect\n"
        "  git CMD       git command\n"
        "  fw            firewall status\n"
        "  fw add RULE   add firewall rule\n"
        "  fw del ID     delete firewall rule\n"
        "  av            antivirus status\n"
        "  av scan PATH  scan path\n"
        "  av qlist      list quarantine\n"
        "  av restore ID restore file\n"
        "  time / date   clock\n"
        "  uptime        time since boot\n"
        "  mem           memory report\n"
        "  cpu           cpu identification\n"
        "  theme NAME    aurora sunset ocean mono\n"
        "  uname         system info\n"
        "  hostname      computer name\n"
        "  whoami        current user\n"
        "  clear         wipe screen\n"
        "  reboot        restart machine\n"
        "  poweroff      switch off\n");
}

static void cmd_kikofetch(void)
{
    char b[96];
    for (int i = 0; i < 4; i++) {
        term_color(mixc(g_accent, rgb(255, 255, 220), 30));
        term_push(LOGO[i]);
        term_color(rgb(200, 206, 218));
        term_push("   ");
        switch (i) {
        case 0: term_push("kikos@kikos-pc"); break;
        case 1: term_push("-----------------"); break;
        case 2:
            strcpy(b, "OS: KiKOS.11 'Aurora'");
            term_color(mixc(g_accent, rgb(255,255,255), 40));
            term_push(b);
            term_color(rgb(200, 206, 218));
            break;
        case 3:
            strcpy(b, "WM: KiWM   Shell: ksh");
            term_push(b);
            break;
        }
        term_push("\n");
    }
    strcpy(b, "Kernel: kikos-1.0-i686  Res: ");
    char n[16];
    utoa_dec((u32)SW, n); strcat(b, n);
    strcat(b, "x"); utoa_dec((u32)SH, n); strcat(b, n);
    term_push(b); term_push("\n");

    u32 up = g_ticks / 100;
    strcpy(b, "Uptime: ");
    utoa_dec(up / 3600, n); strcat(b, n); strcat(b, "h ");
    utoa_dec((up / 60) % 60, n); strcat(b, n); strcat(b, "m ");
    utoa_dec(up % 60, n); strcat(b, n); strcat(b, "s");
    term_push(b); term_push("\n");
    term_push("Type 'about' for the story.\n");
}

static void cmd_about(void)
{
    term_push(
        "\nKiKOS.11 'Aurora' build 2026.08\n"
        "A complete operating system:\n"
        "  own bootloader (two stages, VBE)\n"
        "  own 32-bit kernel + drivers\n"
        "  own window system KiWM\n"
        "  own font KiFont8 and icon set\n\n"
        "crafted with care by y0ungmg\n\n");
}

static void cmd_ver(void)
{
    term_push("KiKOS.11 kernel kikos-1.0-i686 build 2026.08\n");
}

static void cmd_uptime(void)
{
    u32 s = g_ticks / 100;
    char b[64], n[16];
    strcpy(b, "up ");
    utoa_dec(s / 3600, n); strcat(b, n); strcat(b, "h ");
    utoa_dec((s / 60) % 60, n); strcat(b, n); strcat(b, "m ");
    utoa_dec(s % 60, n); strcat(b, n); strcat(b, "s\n");
    term_push(b);
}

static void cmd_mem(void)
{
    char b[80], n[16];
    utoa_dec(total_mem_kb(), n);
    strcpy(b, "RAM: ~"); strcat(b, n); strcat(b, " KB detected\n");
    term_push(b);
    utoa_dec(heap_used() / 1024, n);
    strcpy(b, "Heap in use: "); strcat(b, n); strcat(b, " KB\n");
    term_push(b);
}

static void cmd_cpu(void)
{
    term_push(cpu_brand());
    term_push("\n");
}

static void cmd_mkdir(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: mkdir DIR\n"); return; }
    if (vfs_mkdir_p(arg) == 0) term_push("directory created\n");
    else term_push("mkdir: failed\n");
}

static void cmd_rm(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: rm FILE/DIR\n"); return; }
    if (vfs_rm_r(arg) == 0) term_push("removed\n");
    else term_push("rm: failed\n");
}

static void parse_two_args(const char *arg, char *a, char *b, int max_len) {
    while (*arg == ' ') arg++;
    int i = 0;
    while (*arg && *arg != ' ' && i < max_len - 1) { a[i++] = *arg++; }
    a[i] = 0;
    while (*arg == ' ') arg++;
    i = 0;
    while (*arg && *arg != ' ' && i < max_len - 1) { b[i++] = *arg++; }
    b[i] = 0;
}

static void cmd_cp(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: cp SRC DST\n"); return; }
    char src[256], dst[256];
    parse_two_args(arg, src, dst, 256);
    if (!*src || !*dst) { term_push("usage: cp SRC DST\n"); return; }
    if (vfs_cp(src, dst) == 0) term_push("copied\n");
    else term_push("cp: failed\n");
}

static void cmd_mv(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: mv SRC DST\n"); return; }
    char src[256], dst[256];
    parse_two_args(arg, src, dst, 256);
    if (!*src || !*dst) { term_push("usage: mv SRC DST\n"); return; }
    if (vfs_mv(src, dst) == 0) term_push("moved\n");
    else term_push("mv: failed\n");
}

static void cmd_find(const char *arg)
{
    char path[256] = "/";
    if (arg && *arg) strncpy(path, arg, 255);
    vfs_print_tree(vfs_resolve_path(path), 0);
}

static void cmd_ps(void)
{
    term_push("PID  CPU  MEM    NAME\n");
    term_push("1    2    1024   kikosh\n");
    term_push("2    5    2048   gui\n");
    term_push("3    1    1536   files\n");
    term_push("4    0    512    terminal\n");
    term_push("5    0    256    calculator\n");
    term_push("6    0    1024   doodle\n");
    term_push("7    3    2048   antivirus\n");
    term_push("8    0    768    editor\n");
    term_push("9    1    512    sysmon\n");
    term_push("0    88   0      idle\n");
}

static void cmd_kill(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: kill PID\n"); return; }
    term_push("process killed (simulated)\n");
}

static void cmd_df(void)
{
    u32 total = heap_total() / 1024;
    u32 used = heap_used() / 1024;
    u32 free = total - used;
    char b[64];
    term_push("Filesystem     1K-blocks   Used  Available  Use% Mounted on\n");
    strcpy(b, "/dev/vda1      "); utoa_dec(total, b + 14); 
    strcat(b, "   "); utoa_dec(used, b + strlen(b));
    strcat(b, "   "); utoa_dec(free, b + strlen(b));
    strcat(b, "   "); utoa_dec(used * 100 / total, b + strlen(b));
    strcat(b, "% /\n");
    term_push(b);
}

static void cmd_du(const char *arg)
{
    char path[256] = "/";
    if (arg && *arg) strncpy(path, arg, 255);
    VfsStat st;
    if (vfs_stat(path, &st) == 0) {
        char b[64];
        utoa_dec(st.size / 1024, b);
        term_push(b); term_push(" KB\t"); term_push(path); term_push("\n");
    } else {
        term_push("du: cannot access\n");
    }
}

static void cmd_top(void)
{
    win_open(APP_SYSMON);
    term_push("Opening System Monitor...\n");
}

static void cmd_ping(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: ping HOST\n"); return; }
    char b[64];
    strcpy(b, "PING "); strcat(b, arg); strcat(b, " (simulated)\n");
    term_push(b);
    term_push("64 bytes from "); term_push(arg); term_push(": icmp_seq=1 ttl=64 time=0.5 ms\n");
    term_push("64 bytes from "); term_push(arg); term_push(": icmp_seq=2 ttl=64 time=0.3 ms\n");
    term_push("--- "); term_push(arg); term_push(" ping statistics ---\n");
    term_push("2 packets transmitted, 2 received, 0% packet loss\n");
}

static void cmd_curl(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: curl URL\n"); return; }
    char b[128];
    strcpy(b, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body>"); 
    strcat(b, arg); strcat(b, "</body></html>\n");
    term_push(b);
}

static void cmd_wget(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: wget URL\n"); return; }
    char b[128];
    char fname[64];
    vfs_basename(arg, fname, sizeof(fname));
    if (vfs_write(fname, "<html>downloaded</html>", 21) == 0) {
        strcpy(b, "saved to "); strcat(b, fname); strcat(b, "\n");
    } else {
        strcpy(b, "wget: failed\n");
    }
    term_push(b);
}

static void cmd_ssh(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: ssh USER@HOST\n"); return; }
    char b[64];
    strcpy(b, "Connecting to "); strcat(b, arg); strcat(b, " (simulated)\n");
    term_push(b);
    term_push("Welcome to "); term_push(arg); term_push("\n$ ");
}

static void cmd_git(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: git CMD\n"); return; }
    term_push("git "); term_push(arg); term_push(" (simulated)\n");
}

static void cmd_fw(const char *arg)
{
    if (!arg || !*arg) {
        char buf[512];
        fw_status(buf, sizeof(buf));
        term_push(buf);
        return;
    }
    
    if (!strcmp(arg, "add")) {
        term_push("fw add: use fwctl for advanced rules\n");
        return;
    }
    if (!strcmp(arg, "del")) {
        term_push("fw del: use fwctl for advanced rules\n");
        return;
    }
    if (!strcmp(arg, "enable")) {
        fw_enable();
        term_push("firewall enabled\n");
        return;
    }
    if (!strcmp(arg, "disable")) {
        fw_disable();
        term_push("firewall disabled\n");
        return;
    }
    if (!strcmp(arg, "list")) {
        FwRule *rules[32];
        int count = fw_list_rules(rules, 32);
        term_push("Firewall Rules:\n");
        for (int i = 0; i < count; i++) {
            char b[128];
            snprintf(b, sizeof(b), "  %d: %s -> %s\n",
                i,
                rules[i]->proto == FW_PROTO_TCP ? "TCP" : rules[i]->proto == FW_PROTO_UDP ? "UDP" : "ANY",
                rules[i]->comment);
            term_push(b);
        }
        return;
    }
    term_push("usage: fw [enable|disable|list|add|del]\n");
}

static void cmd_av(const char *arg)
{
    if (!arg || !*arg) {
        char buf[512];
        av_get_status(buf, sizeof(buf));
        term_push(buf);
        return;
    }
    
    if (!strcmp(arg, "scan")) {
        // Not implemented in this simple shell
        term_push("av scan: use avscan for full scan\n");
        return;
    }
    if (!strcmp(arg, "qlist")) {
        AvQuarantine *q[32];
        int count = av_list_quarantine(q, 32);
        term_push("Quarantine:\n");
        for (int i = 0; i < count; i++) {
            char b[128];
            snprintf(b, sizeof(b), "  %d: %s -> %s\n",
                i, q[i]->original_path, q[i]->threat_name);
            term_push(b);
        }
        return;
    }
    if (!strcmp(arg, "restore")) {
        // Would need an ID argument
        term_push("av restore ID\n");
        return;
    }
    if (!strcmp(arg, "enable")) {
        g_antivirus.enabled = 1;
        term_push("antivirus enabled\n");
        return;
    }
    if (!strcmp(arg, "disable")) {
        g_antivirus.enabled = 0;
        term_push("antivirus disabled\n");
        return;
    }
    if (!strcmp(arg, "update")) {
        av_update_db();
        term_push("signature database updated\n");
        return;
    }
    term_push("usage: av [scan|qlist|restore|enable|disable|update]\n");
}

static void cmd_time(void) { char b[24]; format_time(b, sizeof b); term_push(b); term_push("\n"); }
static void cmd_date(void) { char b[24]; format_date(b, sizeof b); term_push(b); term_push("\n"); }

static void cmd_echo(const char *rest)
{
    term_push(rest);
    term_push("\n");
}

static void cmd_ls(const char *arg)
{
    VfsNode *d = vfs_resolve_path(arg && *arg ? arg : "/");
    if (!d || d->type != VFS_DIR) { term_push("ls: no such directory\n"); return; }
    for (int i = 0; i < d->child_count; i++) {
        if (d->children[i]->type == VFS_DIR) term_color(mixc(g_accent, rgb(255,255,255), 40));
        else term_color(rgb(200, 206, 218));
        term_push(d->children[i]->name);
        if (d->children[i]->type == VFS_DIR) term_push("/");
        term_push("   ");
    }
    term_color(rgb(200, 206, 218));
    term_push("\n");
}

static void grep_helper(VfsNode *dir, const char *pat, const char *pathbuf, int depth)
{
    if (depth > 8) return;
    for (int i = 0; i < dir->child_count; i++) {
        VfsNode *n = dir->children[i];
        char p[VFS_MAX_PATH];
        strcpy(p, pathbuf);
        strcat(p, n->name);
        if (n->type == VFS_DIR) {
            strcat(p, "/");
            grep_helper(n, pat, p, depth + 1);
        } else if (n->type == VFS_FILE && n->content) {
            const char *line = n->content;
            while (line && *line) {
                const char *nl = line;
                while (*nl && *nl != '\n') nl++;
                int llen = (int)(nl - line);
                if (llen > 0 && strstr(line, pat)) {
                    term_push(p);
                    term_push(":");
                    char lbuf[132];
                    if (llen > 120) llen = 120;
                    int k;
                    for (k = 0; k < llen; k++) lbuf[k] = line[k];
                    lbuf[llen] = 0;
                    term_push(lbuf);
                    term_push("\n");
                }
                if (!*nl) break;
                line = nl + 1;
            }
        }
    }
}

static void cmd_grep(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: grep PATTERN [PATH]\n"); return; }
    const char *sp = arg;
    while (*sp && *sp != ' ') sp++;
    char pat[64];
    int plen = (int)(sp - arg);
    if (plen > 63) plen = 63;
    int k;
    for (k = 0; k < plen; k++) pat[k] = arg[k];
    pat[plen] = 0;
    const char *path = "/home/kikos";
    if (*sp) { sp++; while (*sp == ' ') sp++; if (*sp) path = sp; }
    VfsNode *d = vfs_resolve_path(path);
    if (!d || d->type != VFS_DIR) { term_push("grep: no such directory\n"); return; }
    char rootp[VFS_MAX_PATH];
    strcpy(rootp, path);
    if (rootp[0] && rootp[strlen(rootp) - 1] != '/') strcat(rootp, "/");
    grep_helper(d, pat, rootp, 0);
}

static void cmd_cat(const char *arg)
{
    if (!arg || !*arg) { term_push("usage: cat FILE\n"); return; }
    char buf[4096];
    int len = vfs_read(arg, buf, sizeof(buf) - 1);
    if (len < 0) { term_push("cat: not found\n"); return; }
    term_push(buf);
    if (len > 0 && buf[len - 1] != '\n') term_push("\n");
}

static void cmd_theme(const char *arg)
{
    if (!strcmp(arg, "aurora")) theme_set(WP_AURORA);
    else if (!strcmp(arg, "sunset")) theme_set(WP_SUNSET);
    else if (!strcmp(arg, "ocean")) theme_set(WP_OCEAN);
    else if (!strcmp(arg, "mono")) theme_set(WP_MONO);
    else { term_color(rgb(235,90,100)); term_push("themes: aurora sunset ocean mono\n"); term_color(rgb(200,206,218)); return; }
    term_color(mixc(g_accent,rgb(255,255,255),40)); term_push("theme applied\n"); term_color(rgb(200,206,218));
}

static void cmd_uname(void)
{
    char b[80], n[16];
    strcpy(b, "KiKOS.11 kikos-1.0-i686 ");
    utoa_dec((u32)SW, n); strcat(b, n); strcat(b, "x");
    utoa_dec((u32)SH, n); strcat(b, n);
    term_push(b); term_push("\n");
}

static void cmd_hostname(void)
{
    term_push("kikos-pc\n");
}

static void cmd_whoami(void)
{
    term_push("kikos\n");
}

static void exec_one(char *line)
{
    while (*line == ' ') line++;
    if (!*line) return;

    char *sp = line;
    while (*sp && *sp != ' ') sp++;
    const char *rest = "";
    if (*sp) { *sp++ = 0; while (*sp == ' ') sp++; rest = sp; }

    if (!strcmp(line, "help")) cmd_help();
    else if (!strcmp(line, "kikofetch")) cmd_kikofetch();
    else if (!strcmp(line, "neofetch")) cmd_kikofetch();
    else if (!strcmp(line, "about")) cmd_about();
    else if (!strcmp(line, "ver")) cmd_ver();
    else if (!strcmp(line, "echo")) cmd_echo(rest);
    else if (!strcmp(line, "ls")) cmd_ls(rest);
    else if (!strcmp(line, "cat")) cmd_cat(rest);
    else if (!strcmp(line, "mkdir")) cmd_mkdir(rest);
    else if (!strcmp(line, "rm")) cmd_rm(rest);
    else if (!strcmp(line, "cp")) cmd_cp(rest);
    else if (!strcmp(line, "mv")) cmd_mv(rest);
    else if (!strcmp(line, "find")) cmd_find(rest);
    else if (!strcmp(line, "grep")) cmd_grep(rest);
    else if (!strcmp(line, "ps")) cmd_ps();
    else if (!strcmp(line, "kill")) cmd_kill(rest);
    else if (!strcmp(line, "df")) cmd_df();
    else if (!strcmp(line, "du")) cmd_du(rest);
    else if (!strcmp(line, "top")) cmd_top();
    else if (!strcmp(line, "ping")) cmd_ping(rest);
    else if (!strcmp(line, "curl")) cmd_curl(rest);
    else if (!strcmp(line, "wget")) cmd_wget(rest);
    else if (!strcmp(line, "ssh")) cmd_ssh(rest);
    else if (!strcmp(line, "git")) cmd_git(rest);
    else if (!strcmp(line, "fw")) cmd_fw(rest);
    else if (!strcmp(line, "av")) cmd_av(rest);
    else if (!strcmp(line, "time")) cmd_time();
    else if (!strcmp(line, "date")) cmd_date();
    else if (!strcmp(line, "uptime")) cmd_uptime();
    else if (!strcmp(line, "mem")) cmd_mem();
    else if (!strcmp(line, "cpu")) cmd_cpu();
    else if (!strcmp(line, "theme")) cmd_theme(rest);
    else if (!strcmp(line, "uname")) cmd_uname();
    else if (!strcmp(line, "hostname")) cmd_hostname();
    else if (!strcmp(line, "whoami")) cmd_whoami();
    else if (!strcmp(line, "clear") || !strcmp(line, "cls")) {
        count = 0;
        memset(lines, 0, sizeof(lines));
    } else if (!strcmp(line, "reboot")) reboot();
    else if (!strcmp(line, "poweroff") || !strcmp(line, "shutdown") ||
             !strcmp(line, "off")) power_off();
    else {
        term_color(rgb(235, 90, 100));
        term_push("unknown command: ");
        term_push(line);
        term_push("\n");
        term_color(rgb(160, 166, 182));
        term_push("type 'help' for commands\n");
    }
}

/* ---------------- app glue ---------------- */

static void submit(void)
{
    term_push(input);
    term_push("\n");
    char copy[128];
    strcpy(copy, input);
    input_len = 0;
    input[0] = 0;
    exec_one(copy);
    term_push("kikos> ");
}

void app_term_key(Window *w, int key)
{
    (void)w;
    if (key == '\b') {
        if (input_len > 0) {
            input[--input_len] = 0;
            ui_request_redraw();
        }
        return;
    }
    if (key == '\n') {
        submit();
        ui_request_redraw();
        return;
    }
    if (key >= 32 && key < 127 && input_len < 120) {
        input[input_len++] = (char)key;
        input[input_len] = 0;
        ui_request_redraw();
    }
}

void app_term_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w; (void)lx; (void)ly; (void)ev;
}

void app_term_draw(Window *w, Rect *c)
{
    fill_rect(c->x, c->y, c->w, c->h, rgb(12, 14, 20));

    int lh = 13;
    int vis = (c->h - 12) / lh;
    if (vis > TERM_MAXL) vis = TERM_MAXL;
    if (vis < 1) return;

    int total = count;
    int skip = total > vis ? total - vis : 0;

    int y = c->y + 6;
    for (int i = 0; i < vis; i++) {
        int rowidx = skip + i;
        if (rowidx >= total) break;
        const char *txt = lines[rowidx];
        text(c->x + 10, y, txt, 1,
             !strncmp(txt, "kikos>", 6) ? mixc(g_accent, rgb(255, 255, 255), 40)
                                        : line_col[rowidx]);
        y += lh;
    }

    if (total + 1 <= vis || skip == 0) {
        static char ibuf[136];
        strcpy(ibuf, "kikos> ");
        strcat(ibuf, input);
        text(c->x + 10, y, ibuf, 1, rgb(220, 226, 236));
        int blink = ((g_ticks / 25) & 1) && w == g_focus;
        if (blink) {
            int cx = c->x + 10 + text_w("kikos> ", 1) + text_w(input, 1);
            fill_rect(cx + 1, y, 7, 9, g_accent);
        }
    }

    if (total > vis) {
        int barh = c->h * vis / total;
        if (barh < 24) barh = 24;
        int maxoff = total - vis;
        int bary = c->y + 4 + (c->h - barh - 8) * (maxoff ? skip : 0) / (maxoff ? maxoff : 1);
        blend_rect(c->x + c->w - 4, bary, 3, barh, g_accent, 150);
    }
}
