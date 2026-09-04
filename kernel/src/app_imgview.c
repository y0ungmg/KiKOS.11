#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"
#include "timer.h"
#include "vfs.h"
#include <string.h>

static int g_img_idx = 0;
static VfsNode *g_img_files[16];
static int g_img_count = 0;
static int g_img_zoom = 100;
static int g_img_drag = 0;
static int g_img_offx = 0, g_img_offy = 0;

static const char *g_img_exts[] = { ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".kimg", 0 };

__attribute__((unused)) static int img_is_image(const char *name) {
    for (int i = 0; g_img_exts[i]; i++) {
        const char *ext = strrchr(name, '.');
        if (ext && strcmp(ext, g_img_exts[i]) == 0) return 1;
    }
    return 0;
}

void app_imgview_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    if (ev == ME_PRESS) {
        if (g_img_drag || (g_img_count > 0 && ly > 40 && ly < 400)) {
            g_img_drag = 1;
            g_img_offx = lx;
            g_img_offy = ly;
        }
    } else if (ev == ME_RELEASE) {
        g_img_drag = 0;
    } else if (ev == ME_MOVE && g_img_drag) {
        // Pan image
    }
}

void app_imgview_key(Window *w, int key) {
    (void)w;
    switch (key) {
    case 0x4B: // Left
        if (g_img_idx > 0) { g_img_idx--; ui_request_redraw(); }
        break;
    case 0x4D: // Right
        if (g_img_idx < g_img_count - 1) { g_img_idx++; ui_request_redraw(); }
        break;
    case 0x48: // Up - zoom in
        if (g_img_zoom < 300) { g_img_zoom += 10; ui_request_redraw(); }
        break;
    case 0x50: // Down - zoom out
        if (g_img_zoom > 25) { g_img_zoom -= 10; ui_request_redraw(); }
        break;
    }
}

static void draw_procedural_image(Rect *c, int x, int y, int w, int h, const char *name) {
    (void)c;
    int cx = x + w/2, cy = y + h/2;

    if (strstr(name, "aurora")) {
        for (int j = 0; j < h; j++) {
            u32 c1 = mixc(rgb(27, 36, 71), rgb(58, 29, 94), (u8)((j * 255) / h));
            hline(x, y + j, w, c1);
        }
        for (int i = 0; i < 5; i++) {
            int px = cx + (rand32() % w) - w/2;
            int py = cy + (rand32() % h) - h/2;
            int r = 50 + rand32() % 50;
            circle_fill(px, py, r, mixc(rgb(100, 200, 255), rgb(180, 100, 255), rand32() % 255));
        }
    } else if (strstr(name, "sunset")) {
        for (int j = 0; j < h; j++) {
            u32 c1 = mixc(rgb(122, 30, 78), rgb(232, 115, 74), (u8)((j * 255) / h));
            hline(x, y + j, w, c1);
        }
        circle_fill(cx, cy + 20, 60, rgb(255, 160, 60));
    } else if (strstr(name, "ocean")) {
        for (int j = 0; j < h; j++) {
            u32 c1 = mixc(rgb(10, 77, 110), rgb(18, 165, 165), (u8)((j * 255) / h));
            hline(x, y + j, w, c1);
        }
        for (int i = 0; i < 3; i++) {
            circle_fill(cx - 40 + i*40, cy, 30, mixc(rgb(40, 180, 220), rgb(20, 220, 180), rand32()%255));
        }
    } else if (strstr(name, "photo")) {
        fill_rect(x, y, w, h, rgb(30, 25, 20));
        for (int i = 0; i < 20; i++) {
            circle_fill(x + rand32()%w, y + rand32()%h, 3 + rand32()%5,
                       mixc(rgb(200, 180, 160), rgb(100, 150, 200), rand32()%255));
        }
    } else if (strstr(name, "screenshot")) {
        fill_rect(x, y, w, h, rgb(18, 20, 28));
        round_rect(x + 20, y + 20, w - 40, h - 40, 8, rgb(28, 30, 40));
        text(x + 40, y + 40, "Desktop Screenshot", 1, g_accent);
        text(x + 40, y + 60, "KiKOS.11 'Aurora'", 1, rgb(160, 168, 184));
    } else {
        // Default pattern
        for (int j = 0; j < h; j++) {
            int r = (x + j) % 256;
            int g = (y + j * 2) % 256;
            int b = (x + y + j) % 256;
            hline(x, y + j, w, rgb(r, g, b));
        }
    }
}

void app_imgview_draw(Window *w, Rect *c) {
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(14, 15, 22));

    // Toolbar
    Rect bar = { c->x, c->y, c->w, 36 };
    round_rect(bar.x, bar.y, bar.w, bar.h, 0, rgb(18, 20, 28));
    hline(bar.x, bar.y + bar.h - 1, bar.w, rgb(45, 49, 62));

    text(bar.x + 12, bar.y + 10, g_img_count > 0 ? g_img_files[g_img_idx]->name : "No images", 1,
         rgb(220, 225, 235));

    const char *btns[] = { "Prev", "Next", "Zoom In", "Zoom Out", "Fit" };
    for (int i = 0; i < 5; i++) {
        Rect btn = { bar.x + bar.w - 70 - i * 68, bar.y + 2, 64, 30 };
        int hover = ui_in(btn, ms_x, ms_y);
        round_rect(btn.x, btn.y, btn.w, btn.h, 4,
                   hover ? mixc(g_accent, rgb(20,22,30), 90) : rgb(28, 32, 44));
        rect_outline(btn.x, btn.y, btn.w + 1, btn.h + 1,
                     hover ? g_accent : rgb(60, 66, 82));
        text(btn.x + btn.w/2 - text_w(btns[i], 1)/2, btn.y + 9, btns[i], 1,
             hover ? rgb(245,248,252) : rgb(200,208,220));
    }

    // Image area
    int img_x = c->x + 20;
    int img_y = c->y + 44;
    int img_w = c->w - 40;
    int img_h = c->h - 52;

    round_rect(img_x, img_y, img_w, img_h, 8, rgb(10, 11, 18));
    rect_outline(img_x, img_y, img_w + 1, img_h + 1, rgb(38, 42, 54));

    if (g_img_count > 0) {
        VfsNode *n = g_img_files[g_img_idx];
        draw_procedural_image(c, img_x + 2, img_y + 2, img_w - 4, img_h - 4, n->name);

        // Info overlay
        char info[64];
        utoa_dec(g_img_idx + 1, info); strcat(info, " / "); utoa_dec(g_img_count, info + strlen(info));
        text(img_x + 12, img_y + 12, info, 1, rgb(200, 208, 220));

        utoa_dec(n->size / 1024, info); strcat(info, " KB");
        text(img_x + 12, img_y + img_h - 20, info, 1, rgb(140, 148, 164));
    } else {
        text(c->x + c->w/2 - text_w("No images in Pictures folder", 1)/2, c->y + c->h/2,
             "No images in Pictures folder", 1, rgb(100, 106, 120));
    }

    // Status
    Rect status = { c->x, c->y + c->h - 24, c->w, 24 };
    fill_rect(status.x, status.y, status.w, status.h, rgb(14, 15, 22));
    hline(status.x, status.y, status.w, rgb(45, 49, 62));
    text(status.x + 10, status.y + 7, "Use arrow keys: Prev/Next, Up/Down: Zoom", 1, rgb(140, 148, 164));
}