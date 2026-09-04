#include "fs.h"

static FsNode docs_kids[] = {
    { "welcome.txt", 0, 0,
      "Welcome to KiKOS.11!\n\n"
      "This whole system - bootloader, kernel,\n"
      "drivers and desktop - was written from\n"
      "scratch for this machine.\n\n"
      "No Linux, no Windows, no borrowed code.\n"
      "Just KiKOS.", 0, 0 },
    { "roadmap.txt", 0, 0,
      "KiKOS roadmap:\n"
      " [x] bootloader (stage1+stage2)\n"
      " [x] VBE 1024x768x32 graphics\n"
      " [x] PS/2 keyboard + mouse\n"
      " [x] KiWM compositor\n"
      " [ ] SMP\n"
      " [ ] native filesystem\n", 0, 0 },
};

static FsNode pics_kids[] = {
    { "aurora.kimg", 0, 1, "", 0, 0 },
    { "sunset.kimg", 0, 1, "", 0, 0 },
    { "ocean.kimg",  0, 1, "", 0, 0 },
};

static FsNode sys_kids[] = {
    { "build.cfg", 0, 0,
      "target=i686-kikos\n"
      "gfx=VBE.1024x768.32\n"
      "wm=KiWM\n"
      "font=KiFont8\n", 0, 0 },
};

static FsNode home_kids[] = {
    { "Documents", 1, 0, 0, docs_kids, 2 },
    { "Pictures", 1, 0, 0, pics_kids, 3 },
    { "System", 1, 0, 0, sys_kids, 1 },
    { "readme.txt", 0, 0,
      "KiKOS.11 'Aurora'\n"
      "build 2026.08\n\n"
      "Try the terminal: kikofetch\n", 0, 0 },
};

FsNode *fs_root(void)
{
    static FsNode root = { "/home/kikos", 1, 0, 0, home_kids, 4 };
    return &root;
}
