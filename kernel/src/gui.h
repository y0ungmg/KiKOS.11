#pragma once

#include "types.h"

typedef struct { int x, y, w, h; } Rect;

enum {
    APP_NONE,
    APP_TERM,
    APP_FILES,
    APP_CALC,
    APP_DOODLE,
    APP_SETTINGS,
    APP_ABOUT,
    APP_AV,
    APP_EDIT,
    APP_SYSMON,
    APP_IMGVIEW,
    APP_MUSIC,
    APP_SNAKE,
    APP_KALEIDOSCOPE,
    APP_GOL,
    APP_SLIDE,
    APP_COUNT
};

enum {
    WP_AURORA = 0,
    WP_SUNSET = 1,
    WP_OCEAN  = 2,
    WP_MONO   = 3
};

enum { ME_PRESS, ME_RELEASE, ME_MOVE };

#define TASKBAR_H 48
#define TITLE_H   30

typedef struct Window {
    int used;
    int visible;
    int maximized;
    int app;
    Rect r;
    Rect saved;
    u32 open_tick;
    u32 anim_tick;
    int anim;          /* 0=none, 1=opening, 2=closing, 3=minimizing */
    char title[24];
} Window;

extern u32 g_accent;
extern int g_theme;
extern int g_clock24;
extern int g_nightlight;
extern int g_auto_theme;
extern int g_focus_mode;
extern int g_mood;
extern volatile u32 g_input_epoch;
extern Window g_wins[16];
extern int g_nwins;
extern Window *g_focus;
extern int g_gui_active;
extern int ms_x, ms_y;
extern int ms_btn_l, ms_btn_r;
extern volatile u8 ms_state_dirty;

void gui_init(void);
void gui_frame(void);
void theme_apply(void);
void theme_set(int t);

Window *win_open(int app);
Window *win_by_app(int app);
Rect win_content(Window *w);

int  ui_button(Rect r, const char *label);
void ui_panel(int x, int y, int w, int h, int r, u32 color, u8 alpha);
int  ui_in(Rect r, int x, int y);
u32  ui_hover_bg(u32 base, Rect r);
void ui_request_redraw(void);
int  gui_handle_key(int key);
int  gui_handle_palette_key(int key);
int  gui_palette_toggle(int key);
void toast(const char *msg);
int  gui_handle_fkey(int key);
