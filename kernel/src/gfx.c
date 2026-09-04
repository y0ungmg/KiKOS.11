#include "gfx.h"
#include "lib.h"

u32 *fb = 0;
int SW = 0, SH = 0;
int PITCH = 0;

static u32 wallpaper[1024 * 768];

static u32 isqrt(u32 v)
{
    u32 res = 0;
    u32 bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

u32 rgb(u8 r, u8 g, u8 b)
{
    return ((u32)r << 16) | ((u32)g << 8) | b;
}

static u32 ch_r(u32 c) { return (c >> 16) & 0xFF; }
static u32 ch_g(u32 c) { return (c >> 8) & 0xFF; }
static u32 ch_b(u32 c) { return c & 0xFF; }

u32 mixc(u32 a, u32 b, u8 t)
{
    u32 inv = 255 - t;
    return rgb((u8)((ch_r(a) * inv + ch_r(b) * t) / 255),
               (u8)((ch_g(a) * inv + ch_g(b) * t) / 255),
               (u8)((ch_b(a) * inv + ch_b(b) * t) / 255));
}

void gfx_init(BootInfo *bi)
{
    fb = (u32 *)bi->fb_addr;
    SW = bi->fb_width;
    SH = bi->fb_height;
    PITCH = bi->fb_pitch / 4;
}

void putpx(int x, int y, u32 c)
{
    if ((unsigned)x >= (unsigned)SW || (unsigned)y >= (unsigned)SH) return;
    fb[(u32)y * PITCH + x] = c;
}

u32 getpx(int x, int y)
{
    if ((unsigned)x >= (unsigned)SW || (unsigned)y >= (unsigned)SH) return 0;
    return fb[(u32)y * PITCH + x];
}

void hline(int x, int y, int w, u32 c)
{
    if (y < 0 || y >= SH) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SW) w = SW - x;
    if (w <= 0) return;
    u32 *p = fb + (u32)y * PITCH + x;
    while (w--) *p++ = c;
}

void vline(int x, int y, int h, u32 c)
{
    for (int j = 0; j < h; j++)
        putpx(x, y + j, c);
}

void fill_rect(int x, int y, int w, int h, u32 c)
{
    for (int j = 0; j < h; j++)
        hline(x, y + j, w, c);
}

void rect_outline(int x, int y, int w, int h, u32 c)
{
    hline(x, y, w, c);
    hline(x, y + h - 1, w, c);
    vline(x, y, h, c);
    vline(x + w - 1, y, h, c);
}

void blend_rect(int x, int y, int w, int h, u32 c, u8 a)
{
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= SH) continue;
        int x0 = x < 0 ? 0 : x;
        int x1 = x + w > SW ? SW : x + w;
        u32 *p = fb + (u32)yy * PITCH;
        for (int i = x0; i < x1; i++)
            p[i] = mixc(p[i], c, a);
    }
}

static void corner_span(int j, int h, int r, int *inset)
{
    *inset = 0;
    if (r <= 0) return;
    int d = -1;
    if (j < r) d = r - j - 1;
    else if (j >= h - r) d = r - (h - 1 - j) - 1;
    if (d >= 0) {
        i32 sq = r * r - d * d;
        u32 rt = isqrt(sq > 0 ? (u32)sq : 0);
        *inset = r - 1 - (int)rt;
        if (*inset < 0) *inset = 0;
    }
}

void gfx_row_inset(int j, int h, int r, int *inset)
{
    if (r > h / 2) r = h / 2;
    corner_span(j, h, r, inset);
}

void round_rect(int x, int y, int w, int h, int r, u32 c)
{
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    for (int j = 0; j < h; j++) {
        int li;
        corner_span(j, h, r, &li);
        hline(x + li, y + j, w - li * 2, c);
    }
}

void round_rect_blend(int x, int y, int w, int h, int r, u32 c, u8 a)
{
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    for (int j = 0; j < h; j++) {
        int li;
        corner_span(j, h, r, &li);
        blend_rect(x + li, y + j, w - li * 2, 1, c, a);
    }
}

void draw_line(int x0, int y0, int x1, int y1, u32 c)
{
    int dx = x1 - x0, dy = y1 - y0;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = dx - dy;
    for (;;) {
        putpx(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void circle_fill(int cx, int cy, int r, u32 c)
{
    for (int j = -r; j <= r; j++) {
        u32 jj = (u32)(j < 0 ? -j : j);
        if (jj * jj > (u32)(r * r)) continue;
        int span = (int)isqrt((u32)r * r - jj * jj);
        hline(cx - span, cy + j, span * 2 + 1, c);
    }
}

void circle_ring(int cx, int cy, int r, u32 c)
{
    i32 f = 1 - r;
    i32 ddfx = 1;
    i32 ddfy = -2 * r;
    i32 x = 0;
    i32 y = r;
    while (x < y) {
        putpx(cx + x, cy + y, c);
        putpx(cx - x, cy + y, c);
        putpx(cx + x, cy - y, c);
        putpx(cx - x, cy - y, c);
        putpx(cx + y, cy + x, c);
        putpx(cx - y, cy + x, c);
        putpx(cx + y, cy - x, c);
        putpx(cx - y, cy - x, c);
        if (f >= 0) { y--; ddfy += 2; f += ddfy; }
        x++;
        ddfx += 2;
        f += ddfx;
    }
}

static double k_sin(double x)
{
    double x2 = x * x;
    return x * (1.0 - x2 / 6.0 * (1.0 - x2 / 20.0 * (1.0 - x2 / 42.0)));
}

static double k_cos(double x)
{
    double x2 = x * x;
    return 1.0 - x2 / 2.0 * (1.0 - x2 / 12.0 * (1.0 - x2 / 30.0));
}

static void norm_angle(double *rad)
{
    const double PI = 3.14159265358979;
    while (*rad > PI) *rad -= 2.0 * PI;
    while (*rad < -PI) *rad += 2.0 * PI;
}

void arc(int cx, int cy, int r, int a0deg, int a1deg, u32 c)
{
    const double DEG = 3.14159265358979 / 180.0;
    for (int a = a0deg; a <= a1deg; a++) {
        double t = a * DEG;
        norm_angle(&t);
        int px = cx + (int)(k_cos(t) * (double)r);
        int py = cy - (int)(k_sin(t) * (double)r);
        putpx(px, py, c);
    }
}

void arc_point(int cx, int cy, int r, int deg, int *px, int *py)
{
    const double DEG = 3.14159265358979 / 180.0;
    double t = deg * DEG;
    norm_angle(&t);
    *px = cx + (int)(k_cos(t) * (double)r);
    *py = cy - (int)(k_sin(t) * (double)r);
}

int font_h(void) { return 8; }

void text(int x, int y, const char *s, int scale, u32 c)
{
    while (*s) {
        char ch = *s++;
        if ((u8)ch < 32 || (u8)ch > 126) continue;
        const u8 *glyph = font8x8[(u8)ch - 32];
        for (int gy = 0; gy < 8; gy++) {
            u8 bits = glyph[gy];
            for (int gx = 0; gx < 8; gx++) {
                if (!(bits & (0x80 >> gx))) continue;
                if (scale == 1) {
                    putpx(x + gx, y + gy, c);
                } else if (scale == 2) {
                    hline(x + gx * 2, y + gy * 2, 2, c);
                    hline(x + gx * 2, y + gy * 2 + 1, 2, c);
                } else {
                    fill_rect(x + gx * scale, y + gy * scale, scale, scale, c);
                }
            }
        }
        x += 8 * scale;
    }
}

void text_blend(int x, int y, const char *s, int scale, u32 c, u8 a)
{
    while (*s) {
        char ch = *s++;
        if ((u8)ch < 32 || (u8)ch > 126) continue;
        const u8 *glyph = font8x8[(u8)ch - 32];
        for (int gy = 0; gy < 8; gy++) {
            u8 bits = glyph[gy];
            for (int gx = 0; gx < 8; gx++) {
                if (!(bits & (0x80 >> gx))) continue;
                blend_rect(x + gx * scale, y + gy * scale, scale, scale, c, a);
            }
        }
        x += 8 * scale;
    }
}

int text_w(const char *s, int scale)
{
    return (int)strlen(s) * 8 * scale;
}

static void glow(int gx, int gy, int rad, u32 color, int strength)
{
    for (int j = gy - rad; j <= gy + rad; j++) {
        if (j < 0 || j >= SH || j >= 768) continue;
        for (int i = gx - rad; i <= gx + rad; i++) {
            if (i < 0 || i >= SW || i >= 1024) continue;
            int dx = i - gx, dy = j - gy;
            u32 d2 = (u32)(dx * dx + dy * dy);
            u32 r2 = (u32)(rad * rad);
            if (d2 > r2) continue;
            u32 fall = (255 - d2 * 255 / r2) * (u32)strength / 255;
            if (fall > 255) fall = 255;
            wallpaper[(u32)j * 1024 + i] = mixc(wallpaper[(u32)j * 1024 + i], color, (u8)fall);
        }
    }
}

void wall_render(int theme)
{
    int w = SW < 1024 ? SW : 1024;
    int h = SH < 768 ? SH : 768;

    u32 c0 = rgb(11, 16, 38), c1 = rgb(27, 36, 71), c2 = rgb(58, 29, 94);
    u32 gA = rgb(53, 224, 218), gB = rgb(200, 107, 250);

    if (theme == 1) {
        c0 = rgb(31, 10, 46); c1 = rgb(122, 30, 78); c2 = rgb(232, 115, 74);
        gA = rgb(255, 170, 80); gB = rgb(255, 90, 140);
    } else if (theme == 2) {
        c0 = rgb(3, 24, 43); c1 = rgb(10, 77, 110); c2 = rgb(18, 165, 165);
        gA = rgb(60, 230, 210); gB = rgb(40, 120, 240);
    } else if (theme == 3) {
        c0 = rgb(20, 22, 27); c1 = rgb(35, 38, 46); c2 = rgb(52, 56, 66);
        gA = rgb(160, 160, 170); gB = rgb(200, 200, 210);
    }

    int cx = w / 2, cy = h / 2;
    u32 maxd = (u32)(cx * cx + cy * cy);
    if (maxd == 0) maxd = 1;

    for (int j = 0; j < h; j++) {
        u32 *rowp = wallpaper + (u32)j * 1024;
        for (int i = 0; i < w; i++) {
            u32 t = ((u32)(i * 7 + j * 10) * 510u) / (u32)((w + h) * 10);
            u32 first = mixc(c0, c1, (u8)(t > 255 ? 255 : t));
            u32 base = t > 255 ? mixc(first, c2, (u8)(t - 255)) : first;
            /* vignette: gentle darkening toward the edges */
            int dx = i - cx, dy = j - cy;
            u32 d2 = (u32)(dx * dx + dy * dy);
            u32 vig = 255 - (d2 * 255) / maxd;
            if (vig < 110) vig = 110;
            base = mixc(base, rgb(0, 0, 0), (u8)(255 - vig));
            rowp[i] = base;
        }
    }

    if (theme == 3) {
        for (int y = 0; y < h; y += 48)
            for (int x = 0; x < w; x++)
                wallpaper[(u32)y * 1024 + x] = mixc(wallpaper[(u32)y * 1024 + x], gA, 12);
        for (int x = 0; x < w; x += 48)
            for (int y = 0; y < h; y++)
                wallpaper[(u32)y * 1024 + x] = mixc(wallpaper[(u32)y * 1024 + x], gA, 12);
    }

    glow(w * 4 / 5, h / 5, h * 2 / 3, gA, 70);
    glow(w / 6, h * 9 / 10, h / 2, gB, 65);
    if (theme != 3) glow(w / 2, h * 7 / 10, h / 3, mixc(gA, gB, 128), 40);

    rand_seed(0xA11CE);
    for (int k = 0; k < 240; k++) {
        int sx = (int)(rand32() % (u32)w);
        int sy = (int)(rand32() % (u32)(h * 2 / 3));
        wallpaper[(u32)sy * 1024 + sx] =
            mixc(wallpaper[(u32)sy * 1024 + sx], rgb(255, 255, 255), 40);
    }
}

void wall_blit(void)
{
    int w = SW < 1024 ? SW : 1024;
    int h = SH < 768 ? SH : 768;

    for (int j = 0; j < SH; j++) {
        if (j < h)
            memcpy(fb + (u32)j * PITCH, wallpaper + (u32)j * 1024, (u32)w * 4);
        else
            memset(fb + (u32)j * PITCH, 0, (u32)SW * 4);
        for (int i = w; j < h && i < SW; i++)
            fb[(u32)j * PITCH + i] = 0;
    }
}
