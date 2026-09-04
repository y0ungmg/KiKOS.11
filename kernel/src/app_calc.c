#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "icons.h"

static double acc = 0, val = 0;
static char op = 0;
static int fresh = 1;
static int err = 0;

static double k_fabs(double v) { return v < 0 ? -v : v; }

static double k_sqrt_d(double v)
{
    if (v <= 0) return 0;
    double x = v > 1 ? v / 2 : 1;
    for (int i = 0; i < 40; i++) {
        double nx = 0.5 * (x + v / x);
        if (k_fabs(nx - x) < 1e-12) break;
        x = nx;
    }
    return x;
}

static const char *btn_labels[5][4] = {
    { "C", "%", "^", "/" },
    { "7", "8", "9", "*" },
    { "4", "5", "6", "-" },
    { "1", "2", "3", "+" },
    { "+/-", "0", ".", "=" },
};

static Rect btn_rect(Rect c, int row, int col)
{
    int cw = (c.w - 24) / 4;
    int chh = (c.h - 74 - 24) / 5;
    if (chh > 44) chh = 44;
    return (Rect){ c.x + 8 + col * (cw + 4), c.y + 74 + row * (chh + 4),
                   cw, chh };
}

static void fmt_val(char *out)
{
    if (err) { strcpy(out, "Error"); return; }
    double v = val;
    if (v != v) { strcpy(out, "NaN"); return; }
    if (v > 999999999.0 || v < -999999999.0) {
        strcpy(out, "Too big");
        return;
    }
    char *p = out;
    if (v < 0) { *p++ = '-'; v = -v; }
    long ipart = (long)v;
    double frac = v - (double)ipart;
    char ib[16];
    utoa_dec((u32)ipart, ib);
    strcpy(p, ib);
    p += strlen(ib);
    frac += 5e-7;
    if (frac >= 1e-6) {
        *p++ = '.';
        for (int d = 0; d < 6; d++) {
            frac *= 10.0;
            int dig = (int)frac;
            *p++ = (char)('0' + dig);
            frac -= (double)dig;
        }
        *p = 0;
        while (p[-1] == '0') *(--p) = 0;
        if (p[-1] == '.') *(--p) = 0;
    } else {
        *p = 0;
    }
}

static void press_key(const char *k)
{
    err = 0;
    char ch = k[0];

    if (ch >= '0' && ch <= '9') {
        if (fresh) { val = 0; fresh = 0; }
        val = val * 10.0 + (double)(ch - '0');
        ui_request_redraw();
        return;
    }

    if (!strcmp(k, "+/-")) { val = -val; fresh = 0; ui_request_redraw(); return; }
    if (!strcmp(k, "^")) {
        if (val >= 0 && !fresh) { val = k_sqrt_d(val); fresh = 1; }
        else if (val < 0) err = 1;
        ui_request_redraw();
        return;
    }

    switch (ch) {
    case '.':
        fresh = 0;
        break;
    case '+': case '-': case '*': case '/':
        if (op && !fresh) {
            switch (op) {
            case '+': acc += val; break;
            case '-': acc -= val; break;
            case '*': acc *= val; break;
            case '/':
                if (val == 0) { err = 1; acc = 0; val = 0; op = 0; fresh = 1; ui_request_redraw(); return; }
                acc /= val;
                break;
            }
            val = acc;
        } else if (!op || !fresh) {
            acc = val;
        }
        op = ch;
        fresh = 1;
        break;
    case '=':
        if (op && !fresh) {
            switch (op) {
            case '+': acc += val; break;
            case '-': acc -= val; break;
            case '*': acc *= val; break;
            case '/':
                if (val == 0) { err = 1; acc = 0; val = 0; op = 0; fresh = 1; ui_request_redraw(); return; }
                acc /= val;
                break;
            }
            val = acc;
            op = 0;
            fresh = 1;
        }
        break;
    case 'C':
        acc = 0; val = 0; op = 0; fresh = 1; err = 0;
        break;
    case '%':
        val = val / 100.0;
        fresh = 1;
        break;
    default:
        break;
    }
    ui_request_redraw();
}

void app_calc_key(Window *w, int key)
{
    (void)w;
    if (key >= '0' && key <= '9') { press_key((char[]){key, 0}); return; }
    switch (key) {
    case '+': press_key("+"); break;
    case '-': press_key("-"); break;
    case '*': press_key("*"); break;
    case '/': press_key("/"); break;
    case '\n': press_key("="); break;
    case '\b': press_key("C"); break;
    default: break;
    }
}

void app_calc_mouse(Window *w, int lx, int ly, int ev)
{
    (void)w;
    if (ev != ME_PRESS) return;
    extern int ms_x, ms_y;
    (void)lx; (void)ly;

    Rect content = win_content(win_by_app(APP_CALC));
    for (int r = 0; r < 5; r++)
        for (int col = 0; col < 4; col++) {
            Rect br = btn_rect(content, r, col);
            int gx = br.x + br.w / 2, gy = br.y + br.h / 2;
            if (ms_x >= br.x && ms_x < br.x + br.w &&
                ms_y >= br.y && ms_y < br.y + br.h) {
                (void)gx; (void)gy;
                press_key(btn_labels[r][col]);
                return;
            }
        }
}

void app_calc_draw(Window *w, Rect *c)
{
    (void)w;
    fill_rect(c->x, c->y, c->w, c->h, rgb(17, 19, 27));

    Rect disp = { c->x + 8, c->y + 8, c->w - 16, 58 };
    round_rect(disp.x, disp.y, disp.w, disp.h, 10, rgb(10, 11, 16));
    rect_outline(disp.x, disp.y, disp.w + 1, disp.h + 1, rgb(58, 63, 80));

    char expr[40];
    expr[0] = 0;
    if (op) {
        char n[20];
        fmt_val(n);
        strcpy(expr, n);
        strcat(expr, " ");
        int el = (int)strlen(expr);
        expr[el++] = op;
        expr[el++] = 0;
    } else {
        strcpy(expr, "");
    }
    text(disp.x + 10, disp.y + 8, expr, 1, rgb(130, 137, 152));

    char shown[40];
    fmt_val(shown);
    int tw = text_w(shown, 2);
    text(disp.x + disp.w - tw - 12, disp.y + disp.h - 22, shown, 2,
         err ? rgb(240, 100, 100) : rgb(235, 238, 246));

    for (int r = 0; r < 5; r++)
        for (int col = 0; col < 4; col++) {
            Rect br = btn_rect(*c, r, col);
            u32 bgc = rgb(36, 39, 50);
            const char *lb = btn_labels[r][col];
            if (lb[0] == '=') bgc = g_accent;
            else if (r == 0) bgc = rgb(30, 33, 43);
            bgc = ui_hover_bg(bgc, br);
            if (ms_btn_l && ui_in(br, ms_x, ms_y)) bgc = mixc(bgc, rgb(255, 255, 255), 35);
            round_rect(br.x, br.y, br.w, br.h, 8, bgc);
            text(br.x + br.w / 2 - text_w(lb, 1) / 2,
                 br.y + br.h / 2 - 4, lb, 1,
                 lb[0] == '=' ? rgb(14, 15, 20) : rgb(225, 229, 238));
        }
}
