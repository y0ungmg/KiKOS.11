#include "pkgmgr.h"
#include "lib.h"
#include "vfs.h"
#include "timer.h"
#include "com1.h"
#include <string.h>

PkgInfo g_pkg_db[PKG_MAX_ENTRIES];
int g_pkg_count = 0;
PkgRepo g_pkg_repos[PKG_REPO_MAX];
int g_repo_count = 0;
char g_pkg_cache_dir[128] = "/var/cache/kikpkg";
char g_pkg_install_root[128] = "/";

static void add_pkg(const char *name, const char *ver, const char *desc,
                    u32 isize, int installed) {
    if (g_pkg_count >= PKG_MAX_ENTRIES) return;
    PkgInfo *p = &g_pkg_db[g_pkg_count];
    memset(p, 0, sizeof(PkgInfo));
    strncpy(p->name, name, PKG_MAX_NAME - 1);
    strncpy(p->version, ver, PKG_MAX_VER - 1);
    strncpy(p->description, desc, 127);
    p->installed_size = isize;
    p->download_size = isize / 4;
    p->installed = installed;
    p->status = installed ? PKG_INSTALLED : PKG_AVAILABLE;
    g_pkg_count++;
}

static void add_dep(const char *pkg, const char *dep) {
    for (int i = 0; i < g_pkg_count; i++) {
        if (strcmp(g_pkg_db[i].name, pkg) == 0) {
            PkgInfo *p = &g_pkg_db[i];
            if (p->dep_count < PKG_MAX_DEPS) {
                strncpy(p->depends[p->dep_count], dep, PKG_MAX_NAME - 1);
                p->dep_count++;
            }
            return;
        }
    }
}

int pkg_init(void) {
    g_pkg_count = 0;
    g_repo_count = 0;

    /* Core system */
    add_pkg("base-system", "1.0.0", "KiKOS.11 base system", 51200, 1);
    add_pkg("kernel", "1.0.0", "KiKOS kernel with VBE", 25600, 1);
    add_pkg("gui", "1.0.0", "KiWM window manager", 102400, 1);
    add_dep("kernel", "base-system");
    add_dep("gui", "kernel");

    /* Applications */
    add_pkg("terminal", "1.0.0", "KiKOS shell", 16384, 1);
    add_pkg("files", "1.0.0", "File manager", 32768, 1);
    add_pkg("calculator", "1.0.0", "Calculator", 8192, 1);
    add_pkg("doodle", "1.0.0", "Paint app", 16384, 1);
    add_pkg("settings", "1.0.0", "System settings", 16384, 1);
    add_pkg("antivirus", "1.0.0", "Antivirus with quarantine", 32768, 1);
    add_pkg("editor", "1.0.0", "Text editor", 16384, 1);
    add_pkg("sysmon", "1.0.0", "System monitor", 16384, 1);
    add_pkg("imgview", "1.0.0", "Image viewer", 16384, 1);
    add_pkg("music", "1.0.0", "Music player", 16384, 1);
    add_dep("terminal", "gui");
    add_dep("files", "gui");
    add_dep("calculator", "gui");

    /* Available packages */
    add_pkg("firefox", "128.0", "Web browser", 204800, 0);
    add_pkg("libreoffice", "24.2", "Office suite", 512000, 0);
    add_pkg("vscode", "1.89", "Code editor", 204800, 0);
    add_dep("firefox", "gui");

    /* Repos */
    PkgRepo *r = &g_pkg_repos[g_repo_count++];
    strcpy(r->name, "kikos-core");
    strcpy(r->url, "https://repo.kikos.org/core");
    r->enabled = 1; r->priority = 10;

    r = &g_pkg_repos[g_repo_count++];
    strcpy(r->name, "kikos-extra");
    strcpy(r->url, "https://repo.kikos.org/extra");
    r->enabled = 1; r->priority = 5;

    vfs_mkdir_p("/var/cache/kikpkg");

    dbg("[kikpkg] initialized\n");
    return 0;
}

int pkg_refresh(void) { return 0; }

int pkg_search(const char *query, PkgInfo **results, int max) {
    int found = 0;
    for (int i = 0; i < g_pkg_count && found < max; i++) {
        if (strstr(g_pkg_db[i].name, query) ||
            strstr(g_pkg_db[i].description, query)) {
            results[found++] = &g_pkg_db[i];
        }
    }
    return found;
}

int pkg_install(const char *name) {
    for (int i = 0; i < g_pkg_count; i++) {
        if (strcmp(g_pkg_db[i].name, name) == 0) {
            if (g_pkg_db[i].installed) return 0;
            /* Install deps first */
            for (int j = 0; j < g_pkg_db[i].dep_count; j++)
                pkg_install(g_pkg_db[i].depends[j]);
            g_pkg_db[i].installed = 1;
            g_pkg_db[i].status = PKG_INSTALLED;
            dbg("[kikpkg] installed "); dbg(name); dbg("\n");
            return 0;
        }
    }
    return -1;
}

int pkg_remove(const char *name) {
    for (int i = 0; i < g_pkg_count; i++) {
        if (strcmp(g_pkg_db[i].name, name) == 0) {
            if (!g_pkg_db[i].installed) return -1;
            /* Check reverse deps */
            for (int j = 0; j < g_pkg_count; j++) {
                if (g_pkg_db[j].installed && j != i) {
                    for (int k = 0; k < g_pkg_db[j].dep_count; k++) {
                        if (strcmp(g_pkg_db[j].depends[k], name) == 0)
                            return -1;
                    }
                }
            }
            g_pkg_db[i].installed = 0;
            g_pkg_db[i].status = PKG_REMOVED;
            dbg("[kikpkg] removed "); dbg(name); dbg("\n");
            return 0;
        }
    }
    return -1;
}

int pkg_update(const char *name) { (void)name; return 0; }

int pkg_list_installed(PkgInfo **list, int max) {
    int count = 0;
    for (int i = 0; i < g_pkg_count && count < max; i++) {
        if (g_pkg_db[i].installed) list[count++] = &g_pkg_db[i];
    }
    return count;
}

int pkg_info(const char *name, PkgInfo *info) {
    for (int i = 0; i < g_pkg_count; i++) {
        if (strcmp(g_pkg_db[i].name, name) == 0) {
            memcpy(info, &g_pkg_db[i], sizeof(PkgInfo));
            return 0;
        }
    }
    return -1;
}

int pkg_add_repo(const char *name, const char *url) {
    if (g_repo_count >= PKG_REPO_MAX) return -1;
    PkgRepo *r = &g_pkg_repos[g_repo_count++];
    strncpy(r->name, name, 47);
    strncpy(r->url, url, 95);
    r->enabled = 1;
    r->priority = 1;
    return 0;
}

int pkg_remove_repo(const char *name) {
    for (int i = 0; i < g_repo_count; i++) {
        if (strcmp(g_pkg_repos[i].name, name) == 0) {
            for (int j = i; j < g_repo_count - 1; j++)
                g_pkg_repos[j] = g_pkg_repos[j + 1];
            g_repo_count--;
            return 0;
        }
    }
    return -1;
}

int pkg_verify(const char *name) { (void)name; return 0; }
int pkg_clean_cache(void) { return 0; }