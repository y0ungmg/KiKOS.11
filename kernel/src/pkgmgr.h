#pragma once

#include "types.h"
#include "vfs.h"

#define PKG_MAX_NAME    64
#define PKG_MAX_VER     32
#define PKG_MAX_DEPS    8
#define PKG_MAX_FILES   16
#define PKG_MAX_ENTRIES 64
#define PKG_REPO_MAX    8

typedef enum {
    PKG_INSTALLED,
    PKG_AVAILABLE,
    PKG_REMOVED,
    PKG_BROKEN
} PkgStatus;

typedef struct {
    char name[PKG_MAX_NAME];
    char version[PKG_MAX_VER];
    char description[128];
    char maintainer[48];
    char url[96];
    char license[32];
    char depends[PKG_MAX_DEPS][PKG_MAX_NAME];
    int dep_count;
    u32 installed_size;
    u32 download_size;
    char checksum[48];
    PkgStatus status;
    int installed;
} PkgInfo;

typedef struct {
    char name[48];
    char url[96];
    int enabled;
    int priority;
} PkgRepo;

extern PkgInfo g_pkg_db[PKG_MAX_ENTRIES];
extern int g_pkg_count;
extern PkgRepo g_pkg_repos[PKG_REPO_MAX];
extern int g_repo_count;
extern char g_pkg_cache_dir[128];
extern char g_pkg_install_root[128];

int pkg_init(void);
int pkg_refresh(void);
int pkg_search(const char *query, PkgInfo **results, int max);
int pkg_install(const char *name);
int pkg_remove(const char *name);
int pkg_update(const char *name);
int pkg_list_installed(PkgInfo **list, int max);
int pkg_info(const char *name, PkgInfo *info);
int pkg_add_repo(const char *name, const char *url);
int pkg_remove_repo(const char *name);
int pkg_verify(const char *name);
int pkg_clean_cache(void);