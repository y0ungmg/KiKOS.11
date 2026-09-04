#pragma once

#include "types.h"

enum {
    ICON_KLOGO,
    ICON_TERM,
    ICON_FILES,
    ICON_CALC,
    ICON_DOODLE,
    ICON_SETTINGS,
    ICON_ABOUT,
    ICON_POWER,
    ICON_REBOOT,
    ICON_FOLDER,
    ICON_TXT,
    ICON_IMG,
    ICON_SPEAKER,
    ICON_NET,
    ICON_SHIELD,
    ICON_IMAGE,
    ICON_MUSIC,
    ICON_SNAKE,
    ICON_WIFI,
    ICON_BATTERY,
    ICON_COUNT
};

void icon_draw(int id, int x, int y, int s);

extern u32 g_accent_a;
extern u32 g_accent_b;
