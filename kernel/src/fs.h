#pragma once

#include "types.h"
#include "gui.h"

typedef struct FsNode {
    const char *name;
    int is_dir;
    int is_img;
    const char *text;
    struct FsNode *kids;
    int nkids;
} FsNode;

FsNode *fs_root(void);
