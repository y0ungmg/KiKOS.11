#pragma once

#include "types.h"

#define VFS_MAX_NAME   64
#define VFS_MAX_PATH   256
#define VFS_MAX_CHILDREN 64
#define VFS_MAX_ENTRIES 1024

typedef enum {
    VFS_FILE,
    VFS_DIR,
    VFS_SYMLINK,
} VfsType;

typedef struct VfsNode {
    char name[VFS_MAX_NAME];
    VfsType type;
    u32 size;
    u32 uid;
    u32 gid;
    u32 mode;
    u32 created;
    u32 modified;
    struct VfsNode *parent;
    struct VfsNode *children[VFS_MAX_CHILDREN];
    int child_count;
    char *content;
    char *link_target;
} VfsNode;

typedef struct VfsStat {
    int type;
    int mode;
    int uid;
    int gid;
    int size;
    int nlink;
    int created;
    int modified;
} VfsStat;

VfsNode *vfs_init(void);
VfsNode *vfs_root(void);
VfsNode *vfs_find(VfsNode *dir, const char *name);
VfsNode *vfs_mkdir(VfsNode *parent, const char *name);
VfsNode *vfs_create(VfsNode *parent, const char *name, const char *content);
VfsNode *vfs_symlink(VfsNode *parent, const char *name, const char *target);
int vfs_remove(VfsNode *parent, const char *name);
int vfs_rename(VfsNode *node, const char *new_name);
void vfs_free(VfsNode *node);
void vfs_print_tree(VfsNode *node, int depth);
char *vfs_get_path(VfsNode *node, char *buf, int buflen);
VfsNode *vfs_resolve_path(const char *path);

int vfs_read(const char *path, char *buf, int buflen);
int vfs_write(const char *path, const char *data, int len);
int vfs_append(const char *path, const char *data, int len);
int vfs_mkdir_p(const char *path);
int vfs_rm_r(const char *path);
int vfs_cp(const char *src, const char *dst);
int vfs_mv(const char *src, const char *dst);
int vfs_stat(const char *path, VfsStat *st);
char *vfs_basename(const char *path, char *buf, int buflen);
char *vfs_dirname(const char *path, char *buf, int buflen);

extern VfsNode *g_vfs_root;