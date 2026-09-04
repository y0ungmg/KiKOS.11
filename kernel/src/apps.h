#pragma once

#include "gui.h"

void app_term_open(Window *w);
void app_term_draw(Window *w, Rect *c);
void app_term_mouse(Window *w, int lx, int ly, int ev);
void app_term_key(Window *w, int key);
void shell_exec(const char *line);

void app_files_draw(Window *w, Rect *c);
void app_files_mouse(Window *w, int lx, int ly, int ev);

void app_calc_draw(Window *w, Rect *c);
void app_calc_mouse(Window *w, int lx, int ly, int ev);
void app_calc_key(Window *w, int key);

void app_doodle_init(void);
void app_doodle_draw(Window *w, Rect *c);
void app_doodle_mouse(Window *w, int lx, int ly, int ev);

void app_settings_draw(Window *w, Rect *c);
void app_settings_mouse(Window *w, int lx, int ly, int ev);

void app_about_draw(Window *w, Rect *c);
void app_about_mouse(Window *w, int lx, int ly, int ev);

void app_av_draw(Window *w, Rect *c);
void app_av_mouse(Window *w, int lx, int ly, int ev);
void app_av_tick(void);

void app_edit_draw(Window *w, Rect *c);
void app_edit_mouse(Window *w, int lx, int ly, int ev);
void app_edit_key(Window *w, int key);

void app_sysmon_draw(Window *w, Rect *c);
void app_sysmon_mouse(Window *w, int lx, int ly, int ev);
void app_sysmon_key(Window *w, int key);

void app_imgview_draw(Window *w, Rect *c);
void app_imgview_mouse(Window *w, int lx, int ly, int ev);
void app_imgview_key(Window *w, int key);

void app_music_draw(Window *w, Rect *c);
void app_music_mouse(Window *w, int lx, int ly, int ev);
void app_music_key(Window *w, int key);

void app_snake_open(Window *w);
void app_snake_draw(Window *w, Rect *c);
void app_snake_mouse(Window *w, int lx, int ly, int ev);
void app_snake_key(Window *w, int key);

void app_kaleido_open(Window *w);
void app_kaleido_draw(Window *w, Rect *c);
void app_kaleido_mouse(Window *w, int lx, int ly, int ev);
void app_kaleido_key(Window *w, int key);

const char *cpu_brand(void);
u32 total_mem_kb(void);
