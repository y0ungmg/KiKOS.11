#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "vfs.h"
#include "timer.h"

#define FILES_NAV_W 180
#define FILES_LIST_W 320
#define FILES_PREVIEW_W 200

static VfsNode *g_files_cur = 0;
static VfsNode *g_files_sel = 0;
static int g_files_sel_tick = 0;
static int g_files_scroll = 0;
static int g_files_nav_sel = -1;
static int g_files_ctx_open = 0;
static int g_files_ctx_x = 0, g_files_ctx_y = 0;
static VfsNode *g_files_ctx_node = 0;

enum {
    FILES_NAV_HOME = 0,
    FILES_NAV_DOCUMENTS,
    FILES_NAV_DOWNLOADS,
    FILES_NAV_PICTURES,
    FILES_NAV_MUSIC,
    FILES_NAV_VIDEOS,
    FILES_NAV_DESKTOP,
    FILES_NAV_ROOT,
    FILES_NAV_COUNT
};

static const struct { const char *label; const char *path; int icon; } g_files_nav[FILES_NAV_COUNT] = {
    { "Home", "/home/kikos", ICON_FOLDER },
    { "Documents", "/home/kikos/Documents", ICON_FOLDER },
    { "Downloads", "/home/kikos/Downloads", ICON_FOLDER },
    { "Pictures", "/home/kikos/Pictures", ICON_IMG },
    { "Music", "/home/kikos/Music", ICON_SPEAKER },
    { "Videos", "/home/kikos/Videos", ICON_IMG },
    { "Desktop", "/home/kikos/Desktop", ICON_FOLDER },
    { "Computer", "/", ICON_FILES },
};

static void files_ensure(void) {
    if (!g_files_cur) g_files_cur = vfs_find(vfs_root(), "home/kikos");
    if (!g_files_cur) g_files_cur = vfs_root();
}

static void files_navigate(VfsNode *dir) {
    if (dir && dir->type == VFS_DIR) {
        g_files_cur = dir;
        g_files_sel = 0;
        g_files_scroll = 0;
        g_files_ctx_open = 0;
        ui_request_redraw();
    }
}

static int files_child_count(VfsNode *dir) {
    return dir ? dir->child_count : 0;
}

static VfsNode *files_get_child(VfsNode *dir, int idx) {
    if (!dir || dir->type != VFS_DIR) return 0;
    if (idx < 0 || idx >= dir->child_count) return 0;
    return dir->children[idx];
}

static char *files_get_path_buf(VfsNode *node, char *buf, int buflen) {
    return vfs_get_path(node, buf, buflen);
}

void app_files_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    files_ensure();

    Rect c = win_content(w);
    int nav_w = FILES_NAV_W;
    int list_x = c.x + nav_w;
    int list_w = c.w - nav_w - FILES_PREVIEW_W;
    (void)list_x; (void)list_w;

    if (g_files_ctx_open) {
        int ctx_w = 180, ctx_h = 140;
        Rect ctx_r = { g_files_ctx_x, g_files_ctx_y, ctx_w, ctx_h };
        if (ev == ME_PRESS && !ui_in(ctx_r, lx, ly)) {
            g_files_ctx_open = 0;
            ui_request_redraw();
            return;
        }
        if (ev == ME_RELEASE && ui_in(ctx_r, lx, ly)) {
            int item_h = 28;
            int idx = (ly - g_files_ctx_y) / item_h;
            if (idx >= 0) {
                // Context menu actions
                if (idx == 0) { /* Open */
                    if (g_files_ctx_node->type == VFS_DIR) files_navigate(g_files_ctx_node);
                    else { /* Open file with appropriate app */ }
                } else if (idx == 1) { /* Open with... */ }
                else if (idx == 2) { /* Rename */ }
                else if (idx == 3) { /* Delete */ }
                else if (idx == 4) { /* Properties */ }
            }
            g_files_ctx_open = 0;
            ui_request_redraw();
        }
        return;
    }

    if (ev == ME_PRESS) {
        // Navigation pane
        if (lx >= c.x && lx < c.x + nav_w) {
            int item_h = 36;
            int idx = (ly - c.y - 40) / item_h;
            if (idx >= 0 && idx < FILES_NAV_COUNT) {
                g_files_nav_sel = idx;
                VfsNode *n = vfs_resolve_path(g_files_nav[idx].path);
                files_navigate(n);
                return;
            }
        }

        // File list
        Rect list_r = { list_x + 4, c.y + 36, list_w - 8, c.h - 100 };
        if (ui_in(list_r, lx, ly)) {
            int item_h = 32;
            int idx = g_files_scroll + (ly - list_r.y) / item_h;
            int count = files_child_count(g_files_cur);
            if (idx >= 0 && idx < count) {
                VfsNode *n = files_get_child(g_files_cur, idx);
                int now = (int)g_ticks;
                if (g_files_sel == n && now - g_files_sel_tick < 40) {
                    // Double click
                    if (n->type == VFS_DIR) {
                        files_navigate(n);
                    }
                } else {
                    g_files_sel = n;
                    g_files_sel_tick = now;
                }
                ui_request_redraw();
            } else {
                g_files_sel = 0;
                ui_request_redraw();
            }
            return;
        }

        // Address bar
        Rect addr = { list_x + 4, c.y + 4, list_w - 8, 28 };
        if (ui_in(addr, lx, ly)) {
            // TODO: edit path
            return;
        }

        // Toolbar buttons
        Rect btn_new = { list_x + list_w - 160, c.y + c.h - 50, 70, 30 };
        if (ui_in(btn_new, lx, ly)) {
            // New file
            return;
        }
        Rect btn_newdir = { list_x + list_w - 80, c.y + c.h - 50, 70, 30 };
        if (ui_in(btn_newdir, lx, ly)) {
            // New folder
            return;
        }
    } else if (ev == ME_RELEASE && ms_btn_r) {
        // Right click for context menu
        Rect list_r = { list_x + 4, c.y + 36, list_w - 8, c.h - 100 };
        if (ui_in(list_r, lx, ly)) {
            int item_h = 32;
            int idx = g_files_scroll + (ly - list_r.y) / item_h;
            int count = files_child_count(g_files_cur);
            if (idx >= 0 && idx < count) {
                VfsNode *n = files_get_child(g_files_cur, idx);
                g_files_sel = n;
                g_files_ctx_node = n;
                g_files_ctx_x = lx;
                g_files_ctx_y = ly;
                g_files_ctx_open = 1;
                ui_request_redraw();
                return;
            }
        }
    }
}

static void files_draw_nav(Window *w, Rect *c) {
    (void)w;
    int nav_x = c->x;
    int nav_w = FILES_NAV_W;

    blend_rect(nav_x, c->y, nav_w, c->h, rgb(10, 12, 18), 230);
    vline(nav_x + nav_w - 1, c->y, c->h, rgb(38, 42, 54));

    text(nav_x + 12, c->y + 14, "Places", 1, rgb(130, 138, 152));

    for (int i = 0; i < FILES_NAV_COUNT; i++) {
        int item_y = c->y + 40 + i * 36;
        Rect ir = { nav_x + 6, item_y, nav_w - 12, 32 };
        int hover = ui_in(ir, ms_x, ms_y);
        int active = (i == g_files_nav_sel);

        u32 bg = hover ? mixc(rgb(255,255,255), rgb(18,20,28), 40) : rgb(18,20,28);
        if (active) bg = mixc(g_accent, rgb(18,20,28), 60);
        round_rect(ir.x, ir.y, ir.w, ir.h, 8, bg);

        icon_draw(g_files_nav[i].icon, ir.x + 8, ir.y + 8, 16);
        text(ir.x + 30, ir.y + 10, g_files_nav[i].label, 1,
             active ? rgb(245,248,252) : (hover ? rgb(220,225,235) : rgb(180,188,202)));
    }
}

static void files_draw_address_bar(Window *w, Rect *c, int list_x, int list_w) {
    (void)w;
    int bar_y = c->y + 4;
    int bar_h = 28;
    Rect bar = { list_x + 4, bar_y, list_w - 8, bar_h };

    round_rect(bar.x, bar.y, bar.w, bar.h, 6, rgb(14, 16, 24));
    rect_outline(bar.x, bar.y, bar.w + 1, bar.h + 1, rgb(50, 56, 72));

    char path_buf[VFS_MAX_PATH];
    files_get_path_buf(g_files_cur, path_buf, sizeof(path_buf));
    text(bar.x + 10, bar.y + 9, path_buf, 1, g_accent);

    // Refresh button
    Rect refresh = { bar.x + bar.w - 32, bar.y + 2, 28, 24 };
    int hover = ui_in(refresh, ms_x, ms_y);
    round_rect(refresh.x, refresh.y, refresh.w, refresh.h, 4,
               hover ? mixc(g_accent, rgb(20,22,30), 80) : rgb(28, 32, 44));
    icon_draw(ICON_SETTINGS, refresh.x + 6, refresh.y + 4, 16);
}

static void files_draw_list(Window *w, Rect *c, int list_x, int list_w) {
    (void)w;
    int list_y = c->y + 36;
    int list_h = c->h - 100;
    Rect list_r = { list_x + 4, list_y, list_w - 8, list_h };

    round_rect(list_r.x, list_r.y, list_r.w, list_r.h, 6, rgb(14, 16, 24));
    rect_outline(list_r.x, list_r.y, list_r.w + 1, list_r.h + 1, rgb(38, 42, 54));

    int count = files_child_count(g_files_cur);
    int item_h = 32;
    int visible = (list_r.h - 4) / item_h;

    // Scrollbar
    if (count > visible) {
        int sb_x = list_r.x + list_r.w - 10;
        int sb_h = list_r.h - 4;
        int thumb_h = (visible * sb_h) / count;
        int thumb_y = (g_files_scroll * (sb_h - thumb_h)) / (count - visible);
        round_rect(sb_x, list_r.y + 2 + thumb_y, 6, thumb_h, 3, rgb(80, 88, 104));
    }

    for (int i = 0; i < visible; i++) {
        int idx = g_files_scroll + i;
        if (idx >= count) break;

        VfsNode *n = files_get_child(g_files_cur, idx);
        int item_y = list_r.y + 2 + i * item_h;
        Rect ir = { list_r.x + 2, item_y, list_r.w - 14, item_h };

        int sel = (g_files_sel == n);
        int hover = ui_in(ir, ms_x, ms_y) && !g_files_ctx_open;

        u32 bg = hover ? mixc(rgb(255,255,255), rgb(18,20,28), 30) : rgb(18,20,28);
        if (sel) bg = mixc(g_accent, rgb(18,20,28), 70);
        round_rect(ir.x, ir.y, ir.w, ir.h, 6, bg);

        int icon = n->type == VFS_DIR ? ICON_FOLDER :
                   n->type == VFS_SYMLINK ? ICON_TXT : ICON_TXT;

        // Determine icon by extension for files
        if (n->type == VFS_FILE) {
            const char *ext = strrchr(n->name, '.');
            if (ext) {
                if (strcmp(ext, ".png")==0 || strcmp(ext, ".jpg")==0 || strcmp(ext, ".jpeg")==0 ||
                    strcmp(ext, ".bmp")==0 || strcmp(ext, ".gif")==0 || strcmp(ext, ".kimg")==0)
                    icon = ICON_IMG;
                else if (strcmp(ext, ".txt")==0 || strcmp(ext, ".md")==0 || strcmp(ext, ".log")==0)
                    icon = ICON_TXT;
                else if (strcmp(ext, ".ogg")==0 || strcmp(ext, ".mp3")==0 || strcmp(ext, ".wav")==0)
                    icon = ICON_SPEAKER;
                else if (strcmp(ext, ".mp4")==0 || strcmp(ext, ".avi")==0 || strcmp(ext, ".mov")==0)
                    icon = ICON_IMG;
            }
        }

        icon_draw(icon, ir.x + 8, ir.y + 7, 18);

        u32 txt_c = sel ? rgb(245,248,252) : (hover ? rgb(220,225,235) : rgb(200,208,220));
        text(ir.x + 32, ir.y + 9, n->name, 1, txt_c);

        // Size and modified time
        char info[64];
        if (n->type == VFS_FILE) {
            utoa_dec(n->size, info);
            strcat(info, " bytes");
        } else {
            utoa_dec(n->child_count, info);
            strcat(info, " items");
        }
        text(ir.x + ir.w - 8 - text_w(info, 1), ir.y + 9, info, 1, rgb(120, 128, 144));
    }

    if (count == 0) {
        text(list_r.x + 12, list_r.y + list_r.h / 2 - 4,
             "(empty)", 1, rgb(100, 106, 120));
    }
}

static void files_draw_preview(Window *w, Rect *c, int preview_x, int preview_w) {
    (void)w;
    int preview_y = c->y + 4;
    int preview_h = c->h - 8;
    Rect pv = { preview_x + 4, preview_y, preview_w - 8, preview_h };

    round_rect(pv.x, pv.y, pv.w, pv.h, 10, rgb(14, 16, 24));
    rect_outline(pv.x, pv.y, pv.w + 1, pv.h + 1, rgb(45, 50, 64));

    if (!g_files_sel) {
        text(pv.x + 12, pv.y + pv.h / 2 - 4,
             "Select a file\nto preview", 1, rgb(100, 106, 120));
        return;
    }

    VfsNode *n = g_files_sel;
    icon_draw(n->type == VFS_DIR ? ICON_FOLDER :
              n->type == VFS_SYMLINK ? ICON_TXT : ICON_TXT,
              pv.x + 12, pv.y + 12, 48);

    text(pv.x + 70, pv.y + 18, n->name, 1, rgb(235, 240, 248));

    char info[128];
    int y = pv.y + 50;
    utoa_dec(n->size, info);
    if (n->type == VFS_FILE) strcat(info, " bytes");
    else strcat(info, " bytes");
    text(pv.x + 12, y, info, 1, rgb(160, 168, 184)); y += 16;

    utoa_dec(n->modified / 100, info);
    strcat(info, "s ago modified");
    text(pv.x + 12, y, info, 1, rgb(130, 138, 152)); y += 16;

    if (n->type == VFS_FILE) {
        const char *ext = strrchr(n->name, '.');
        if (ext) {
            strcpy(info, "Type: ");
            strcat(info, ext + 1);
            text(pv.x + 12, y, info, 1, rgb(130, 138, 152)); y += 16;
        }
    }

    // Preview content for text files
    if (n->type == VFS_FILE && n->content) {
        y += 10;
        round_rect(pv.x + 8, y, pv.w - 20, pv.h - (y - pv.y) - 12, 6, rgb(8, 10, 16));
        const char *t = n->content;
        int line_y = y + 6;
        while (*t && line_y < pv.y + pv.h - 18) {
            const char *nl = t;
            while (*nl && *nl != '\n') nl++;
            u32 len = (u32)(nl - t);
            if (len > 55) len = 55;
            char lineb[64];
            memcpy(lineb, t, len);
            lineb[len] = 0;
            text(pv.x + 14, line_y, lineb, 1, rgb(170, 180, 196));
            line_y += 12;
            t = *nl ? nl + 1 : nl;
        }
    }
}

static void files_draw_toolbar(Window *w, Rect *c, int list_x, int list_w) {
    (void)w;
    int bar_y = c->y + c->h - 50;
    int btn_w = 70, btn_h = 30, gap = 10;
    int start_x = list_x + list_w - btn_w * 2 - gap;

    // New file
    Rect btn1 = { start_x, bar_y, btn_w, btn_h };
    int hover1 = ui_in(btn1, ms_x, ms_y);
    round_rect(btn1.x, btn1.y, btn1.w, btn1.h, 6,
               hover1 ? mixc(g_accent, rgb(20,22,30), 90) : rgb(28, 32, 44));
    rect_outline(btn1.x, btn1.y, btn1.w + 1, btn1.h + 1,
                 hover1 ? g_accent : rgb(60, 66, 82));
    text(btn1.x + btn1.w/2 - text_w("New File", 1)/2, btn1.y + 9, "New File", 1,
         hover1 ? rgb(245,248,252) : rgb(200,208,220));

    // New folder
    Rect btn2 = { start_x + btn_w + gap, bar_y, btn_w, btn_h };
    int hover2 = ui_in(btn2, ms_x, ms_y);
    round_rect(btn2.x, btn2.y, btn2.w, btn2.h, 6,
               hover2 ? mixc(g_accent, rgb(20,22,30), 90) : rgb(28, 32, 44));
    rect_outline(btn2.x, btn2.y, btn2.w + 1, btn2.h + 1,
                 hover2 ? g_accent : rgb(60, 66, 82));
    text(btn2.x + btn2.w/2 - text_w("New Folder", 1)/2, btn2.y + 9, "New Folder", 1,
         hover2 ? rgb(245,248,252) : rgb(200,208,220));
}

static void files_draw_context_menu(void) {
    if (!g_files_ctx_open) return;

    int ctx_w = 180, ctx_h = 140;
    int cx = g_files_ctx_x;
    int cy = g_files_ctx_y;

    // Clamp to screen
    if (cx + ctx_w > SW) cx = SW - ctx_w;
    if (cy + ctx_h > SH - 44) cy = SH - 44 - ctx_h;

    round_rect(cx, cy, ctx_w, ctx_h, 8, rgb(19, 21, 30));
    rect_outline(cx, cy, ctx_w + 1, ctx_h + 1, rgb(60, 66, 82));

    const char *items[] = { "Open", "Open With...", "Rename", "Delete", "Properties" };
    int item_h = 28;
    for (int i = 0; i < 5; i++) {
        Rect ir = { cx + 4, cy + 4 + i * item_h, ctx_w - 8, item_h };
        int hover = ui_in(ir, ms_x, ms_y);
        if (hover)
            round_rect(ir.x, ir.y, ir.w, ir.h, 6,
                       mixc(rgb(255,255,255), rgb(18,20,28), 40));
        text(ir.x + 12, ir.y + 9, items[i], 1,
             hover ? rgb(245,248,252) : rgb(200,208,220));
    }
}

void app_files_draw(Window *w, Rect *c) {
    fill_rect(c->x, c->y, c->w, c->h, rgb(19, 21, 29));

    int nav_w = FILES_NAV_W;
    int list_x = c->x + nav_w;
    int list_w = c->w - nav_w - FILES_PREVIEW_W;
    int preview_x = list_x + list_w;
    int preview_w = FILES_PREVIEW_W;

    // Vertical separators
    vline(list_x - 1, c->y, c->h, rgb(38, 42, 54));
    vline(preview_x - 1, c->y, c->h, rgb(38, 42, 54));

    files_draw_nav(w, c);
    files_draw_address_bar(w, c, list_x, list_w);
    files_draw_list(w, c, list_x, list_w);
    files_draw_preview(w, c, preview_x, preview_w);
    files_draw_toolbar(w, c, list_x, list_w);
    files_draw_context_menu();
}