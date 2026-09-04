#include "apps.h"
#include "gfx.h"
#include "lib.h"
#include "gui.h"
#include "timer.h"

#define EDIT_MAX_LINES  512
#define EDIT_MAX_LINELEN 256

static char g_edit_lines[EDIT_MAX_LINES][EDIT_MAX_LINELEN];
static int g_edit_line_count = 0;
static int g_edit_cursor_x = 0;
static int g_edit_cursor_y = 0;
static int g_edit_scroll = 0;
static int g_edit_dirty = 0;
static char g_edit_filename[64] = "untitled.txt";
static u32 edit_syntax_color(const char *line, int col) {
    char word[64];
    int wi = 0;
    int start = col;
    while (start > 0 && (line[start-1] == '_' || (line[start-1] >= 'a' && line[start-1] <= 'z') || (line[start-1] >= 'A' && line[start-1] <= 'Z') || (line[start-1] >= '0' && line[start-1] <= '9'))) start--;
    for (int i = start; i < col && wi < 63 && line[i]; i++) word[wi++] = line[i];
    word[wi] = 0;
    if (wi == 0) return rgb(200, 210, 220);

    if (strcmp(word, "if")==0 || strcmp(word, "else")==0 || strcmp(word, "while")==0 ||
        strcmp(word, "for")==0 || strcmp(word, "return")==0 || strcmp(word, "switch")==0 ||
        strcmp(word, "case")==0 || strcmp(word, "default")==0 || strcmp(word, "break")==0 ||
        strcmp(word, "continue")==0 || strcmp(word, "goto")==0)
        return rgb(200, 150, 255);
    if (strcmp(word, "int")==0 || strcmp(word, "char")==0 || strcmp(word, "void")==0 ||
        strcmp(word, "float")==0 || strcmp(word, "double")==0 || strcmp(word, "struct")==0 ||
        strcmp(word, "union")==0 || strcmp(word, "enum")==0 || strcmp(word, "typedef")==0 ||
        strcmp(word, "static")==0 || strcmp(word, "const")==0 || strcmp(word, "volatile")==0 ||
        strcmp(word, "unsigned")==0 || strcmp(word, "signed")==0 || strcmp(word, "short")==0 ||
        strcmp(word, "long")==0)
        return rgb(100, 200, 255);
    if (strcmp(word, "true")==0 || strcmp(word, "false")==0 || strcmp(word, "NULL")==0)
        return rgb(255, 180, 100);
    if (word[0] >= '0' && word[0] <= '9')
        return rgb(100, 255, 180);
    if (word[0] == '"')
        return rgb(255, 220, 100);
    return rgb(200, 210, 220);
}

__attribute__((unused)) static void edit_load_default(void) {
    (void)0;
}

__attribute__((unused)) static void edit_new_file(void) {
    (void)0;
}

void app_edit_mouse(Window *w, int lx, int ly, int ev) {
    (void)w;
    if (ev == ME_PRESS) {
        int line_h = 18;
        int line = ly / line_h + g_edit_scroll;
        if (line >= 0 && line < g_edit_line_count) {
            g_edit_cursor_y = line;
            int len = strlen(g_edit_lines[line]);
            g_edit_cursor_x = lx / 8;
            if (g_edit_cursor_x > len) g_edit_cursor_x = len;
        }
    }
}

void app_edit_key(Window *w, int key) {
    (void)w;
    switch (key) {
    case 0xC8: // Up
        if (g_edit_cursor_y > 0) {
            g_edit_cursor_y--;
            if (g_edit_cursor_y < g_edit_scroll) g_edit_scroll = g_edit_cursor_y;
            int len = strlen(g_edit_lines[g_edit_cursor_y]);
            if (g_edit_cursor_x > len) g_edit_cursor_x = len;
        }
        break;
    case 0xD0: // Down
        if (g_edit_cursor_y < g_edit_line_count - 1) {
            g_edit_cursor_y++;
            if (g_edit_cursor_y >= g_edit_scroll + 25) g_edit_scroll = g_edit_cursor_y - 24;
            int len = strlen(g_edit_lines[g_edit_cursor_y]);
            if (g_edit_cursor_x > len) g_edit_cursor_x = len;
        }
        break;
    case 0xCB: // Left
        if (g_edit_cursor_x > 0) g_edit_cursor_x--;
        else if (g_edit_cursor_y > 0) {
            g_edit_cursor_y--;
            g_edit_cursor_x = strlen(g_edit_lines[g_edit_cursor_y]);
        }
        break;
    case 0xCD: // Right
        if (g_edit_cursor_x < (int)strlen(g_edit_lines[g_edit_cursor_y])) g_edit_cursor_x++;
        else if (g_edit_cursor_y < g_edit_line_count - 1) {
            g_edit_cursor_y++;
            g_edit_cursor_x = 0;
        }
        break;
    case '\b': // Backspace
        if (g_edit_cursor_x > 0) {
            char *line = g_edit_lines[g_edit_cursor_y];
            int len = strlen(line);
            for (int i = g_edit_cursor_x - 1; i < len; i++) line[i] = line[i+1];
            g_edit_cursor_x--;
            g_edit_dirty = 1;
        } else if (g_edit_cursor_y > 0) {
            int prev_len = strlen(g_edit_lines[g_edit_cursor_y - 1]);
            strcat(g_edit_lines[g_edit_cursor_y - 1], g_edit_lines[g_edit_cursor_y]);
            for (int i = g_edit_cursor_y; i < g_edit_line_count - 1; i++)
                strcpy(g_edit_lines[i], g_edit_lines[i+1]);
            g_edit_line_count--;
            g_edit_cursor_y--;
            g_edit_cursor_x = prev_len;
            g_edit_dirty = 1;
        }
        break;
    case '\n': // Enter
        if (g_edit_line_count < EDIT_MAX_LINES) {
            for (int i = g_edit_line_count; i > g_edit_cursor_y + 1; i--)
                strcpy(g_edit_lines[i], g_edit_lines[i-1]);
            int cur_len = strlen(g_edit_lines[g_edit_cursor_y]);
            if (g_edit_cursor_x < cur_len) {
                strcpy(g_edit_lines[g_edit_cursor_y + 1], g_edit_lines[g_edit_cursor_y] + g_edit_cursor_x);
                g_edit_lines[g_edit_cursor_y][g_edit_cursor_x] = 0;
            } else {
                g_edit_lines[g_edit_cursor_y + 1][0] = 0;
            }
            g_edit_line_count++;
            g_edit_cursor_y++;
            g_edit_cursor_x = 0;
            g_edit_dirty = 1;
        }
        break;
    default:
        if (key >= 32 && key < 127) {
            char *line = g_edit_lines[g_edit_cursor_y];
            int len = strlen(line);
            if (len < EDIT_MAX_LINELEN - 1) {
                for (int i = len; i >= g_edit_cursor_x; i--) line[i+1] = line[i];
                line[g_edit_cursor_x] = (char)key;
                g_edit_cursor_x++;
                g_edit_dirty = 1;
            }
        }
        break;
    }
    ui_request_redraw();
}

void app_edit_draw(Window *w, Rect *c) {
    (void)w;
    if (g_edit_line_count == 0) {
        g_edit_line_count = 12;
        strcpy(g_edit_lines[0], "// KiKOS.11 Text Editor");
        strcpy(g_edit_lines[1], "// File: untitled.txt");
        strcpy(g_edit_lines[2], "");
        strcpy(g_edit_lines[3], "#include <stdio.h>");
        strcpy(g_edit_lines[4], "");
        strcpy(g_edit_lines[5], "int main() {");
        strcpy(g_edit_lines[6], "    printf(\"Hello, KiKOS!\\n\");");
        strcpy(g_edit_lines[7], "    return 0;");
        strcpy(g_edit_lines[8], "}");
        strcpy(g_edit_lines[9], "");
        strcpy(g_edit_lines[10], "// Edit this file and press Ctrl+S to save");
        strcpy(g_edit_lines[11], "");
    }
    fill_rect(c->x, c->y, c->w, c->h, rgb(18, 20, 28));

    // Line numbers
    blend_rect(c->x, c->y, 48, c->h, rgb(14, 15, 22), 240);
    vline(c->x + 47, c->y, c->h, rgb(38, 42, 54));

    // Toolbar
    Rect bar = { c->x + 48, c->y, c->w - 48, 32 };
    round_rect(bar.x, bar.y, bar.w, bar.h, 0, rgb(22, 24, 34));
    hline(bar.x, bar.y + bar.h - 1, bar.w, rgb(45, 49, 62));

    text(bar.x + 10, bar.y + 10, g_edit_filename, 1, g_edit_dirty ? rgb(255, 180, 60) : rgb(180, 188, 202));
    if (g_edit_dirty) text(bar.x + 10 + text_w(g_edit_filename, 1) + 4, bar.y + 10, "●", 1, rgb(255, 100, 60));

    // Buttons
    const char *btns[] = { "New", "Open", "Save", "Save As" };
    for (int i = 0; i < 4; i++) {
        Rect btn = { bar.x + bar.w - 70 - i * 65, bar.y + 2, 60, 26 };
        int hover = ui_in(btn, ms_x, ms_y);
        round_rect(btn.x, btn.y, btn.w, btn.h, 4,
                   hover ? mixc(g_accent, rgb(20,22,30), 90) : rgb(28, 32, 44));
        rect_outline(btn.x, btn.y, btn.w + 1, btn.h + 1,
                     hover ? g_accent : rgb(60, 66, 82));
        text(btn.x + btn.w/2 - text_w(btns[i], 1)/2, btn.y + 8, btns[i], 1,
             hover ? rgb(245,248,252) : rgb(200,208,220));
    }

    // Editor area
    int edit_y = c->y + 34;
    int edit_h = c->h - 34;
    int line_h = 18;
    int visible = edit_h / line_h;

    for (int i = 0; i < visible; i++) {
        int line_idx = g_edit_scroll + i;
        int y = edit_y + i * line_h;
        if (y + line_h > c->y + c->h) break;

        // Line number
        if (line_idx < g_edit_line_count) {
            char num[8];
            utoa_dec(line_idx + 1, num);
            text(c->x + 44 - text_w(num, 1), y + 2, num, 1, rgb(90, 98, 112));
        }

        // Current line highlight
        if (line_idx == g_edit_cursor_y) {
            fill_rect(c->x + 50, y, c->w - 52, line_h, mixc(g_accent, rgb(0,0,0), 20));
        }

        if (line_idx < g_edit_line_count) {
            const char *line = g_edit_lines[line_idx];
            int x = c->x + 54;
            for (int j = 0; line[j] && x < c->x + c->w - 10; j++) {
                u32 color = edit_syntax_color(line, j);
                char ch[2] = { line[j], 0 };
                text(x, y + 2, ch, 1, color);
                x += 8;
            }
        }
    }

    // Cursor
    if ((g_ticks / 50) % 2 == 0) {
        int cy = edit_y + (g_edit_cursor_y - g_edit_scroll) * line_h;
        int cx = c->x + 54 + g_edit_cursor_x * 8;
        if (g_edit_cursor_y >= g_edit_scroll && g_edit_cursor_y < g_edit_scroll + visible)
            vline(cx, cy + 2, line_h - 4, g_accent);
    }

    // Scrollbar
    if (g_edit_line_count > visible) {
        int sb_x = c->x + c->w - 10;
        int sb_h = edit_h;
        int thumb_h = (visible * sb_h) / g_edit_line_count;
        int thumb_y = (g_edit_scroll * (sb_h - thumb_h)) / (g_edit_line_count - visible);
        round_rect(sb_x, edit_y + thumb_y, 6, thumb_h, 3, rgb(80, 88, 104));
    }

    // Status bar
    Rect status = { c->x, c->y + c->h - 24, c->w, 24 };
    fill_rect(status.x, status.y, status.w, status.h, rgb(14, 15, 22));
    hline(status.x, status.y, status.w, rgb(45, 49, 62));
    char status_text[64];
    utoa_dec(g_edit_cursor_y + 1, status_text);
    strcat(status_text, ":");
    utoa_dec(g_edit_cursor_x + 1, status_text + strlen(status_text));
    strcat(status_text, "  Ln ");
    utoa_dec(g_edit_line_count, status_text + strlen(status_text));
    text(status.x + 10, status.y + 7, status_text, 1, rgb(140, 148, 164));
    text(status.x + status.w - 80, status.y + 7, "UTF-8  LF", 1, rgb(100, 108, 124));
}