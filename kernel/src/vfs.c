#include "vfs.h"
#include "lib.h"
#include "timer.h"
#include "com1.h"
#include "mm.h"

VfsNode g_vfs_nodes[VFS_MAX_ENTRIES];
int g_vfs_count = 0;
VfsNode *g_vfs_root = 0;

static VfsNode *vfs_alloc_node(const char *name, VfsType type, VfsNode *parent) {
    if (g_vfs_count >= VFS_MAX_ENTRIES) return 0;
    VfsNode *n = &g_vfs_nodes[g_vfs_count++];
    memset(n, 0, sizeof(VfsNode));
    strncpy(n->name, name, VFS_MAX_NAME - 1);
    n->type = type;
    n->parent = parent;
    n->uid = 1000;
    n->gid = 1000;
    n->created = g_ticks;
    n->modified = g_ticks;
    if (type == VFS_DIR) {
        n->mode = 0755;
        n->size = 4096;
    } else {
        n->mode = 0644;
    }
    if (parent && parent->child_count < VFS_MAX_CHILDREN) {
        parent->children[parent->child_count++] = n;
    }
    return n;
}

VfsNode *vfs_init(void) {
    g_vfs_count = 0;
    g_vfs_root = vfs_alloc_node("/", VFS_DIR, 0);
    g_vfs_root->name[0] = '/'; g_vfs_root->name[1] = 0;
    g_vfs_root->parent = g_vfs_root;

    VfsNode *bin = vfs_mkdir(g_vfs_root, "bin");
    vfs_create(bin, "ls", "ELF binary");
    vfs_create(bin, "cat", "ELF binary");
    vfs_create(bin, "grep", "ELF binary");
    vfs_create(bin, "vim", "ELF binary");
    vfs_create(bin, "nano", "ELF binary");
    vfs_create(bin, "sudo", "ELF binary");
    vfs_create(bin, "reboot", "ELF binary");
    vfs_create(bin, "shutdown", "ELF binary");
    vfs_create(bin, "mkdir", "ELF binary");
    vfs_create(bin, "rm", "ELF binary");
    vfs_create(bin, "cp", "ELF binary");
    vfs_create(bin, "mv", "ELF binary");
    vfs_create(bin, "find", "ELF binary");
    vfs_create(bin, "ps", "ELF binary");
    vfs_create(bin, "kill", "ELF binary");
    vfs_create(bin, "df", "ELF binary");
    vfs_create(bin, "du", "ELF binary");
    vfs_create(bin, "top", "ELF binary");
    vfs_create(bin, "ping", "ELF binary");
    vfs_create(bin, "curl", "ELF binary");
    vfs_create(bin, "wget", "ELF binary");
    vfs_create(bin, "ssh", "ELF binary");
    vfs_create(bin, "git", "ELF binary");
    vfs_create(bin, "kikpkg", "ELF binary");
    vfs_create(bin, "fwctl", "ELF binary");
    vfs_create(bin, "avscan", "ELF binary");
    vfs_create(bin, "imgview", "ELF binary");
    vfs_create(bin, "music", "ELF binary");
    vfs_create(bin, "sysmon", "ELF binary");
    vfs_create(bin, "settings", "ELF binary");
    vfs_create(bin, "calc", "ELF binary");
    vfs_create(bin, "doodle", "ELF binary");
    vfs_create(bin, "edit", "ELF binary");
    vfs_create(bin, "terminal", "ELF binary");
    vfs_create(bin, "files", "ELF binary");
    vfs_create(bin, "about", "ELF binary");
    vfs_create(bin, "av", "ELF binary");

    VfsNode *boot = vfs_mkdir(g_vfs_root, "boot");
    vfs_create(boot, "stage1.bin", "MBR bootloader");
    vfs_create(boot, "stage2.bin", "VBE bootloader");
    vfs_create(boot, "kernel.bin", "Kernel image");
    vfs_create(boot, "grub.cfg", "timeout=3\ndefault=kikos\n");

    VfsNode *dev = vfs_mkdir(g_vfs_root, "dev");
    vfs_create(dev, "null", "");
    vfs_create(dev, "zero", "");
    vfs_create(dev, "random", "");
    vfs_create(dev, "tty0", "");
    vfs_create(dev, "fb0", "");
    vfs_create(dev, "mouse0", "");
    vfs_create(dev, "kbd0", "");

    VfsNode *etc = vfs_mkdir(g_vfs_root, "etc");
    vfs_create(etc, "kikos.conf", "[system]\ntheme=aurora\naccent=teal\nlanguage=en_US\n");
    vfs_create(etc, "passwd", "root:x:0:0:root:/root:/bin/kikosh\nkikos:x:1000:1000:KiKOS User:/home/kikos:/bin/kikosh\n");
    vfs_create(etc, "hosts", "127.0.0.1\tlocalhost\n::1\t\tlocalhost\n");
    vfs_create(etc, "fstab", "/dev/sda1\t/\text4\tdefaults\t0 1\n");
    vfs_create(etc, "resolv.conf", "nameserver 1.1.1.1\nnameserver 8.8.8.8\n");
    VfsNode *skel = vfs_mkdir(etc, "skel");
    vfs_create(skel, ".profile", "export PATH=/bin:/usr/bin\n");

    VfsNode *home = vfs_mkdir(g_vfs_root, "home");
    VfsNode *kikos_home = vfs_mkdir(home, "kikos");
    VfsNode *desktop = vfs_mkdir(kikos_home, "Desktop");
    vfs_create(desktop, "Terminal.lnk", "[Shortcut]\nTarget=/bin/kikosh\nIcon=terminal\n");
    vfs_create(desktop, "Files.lnk", "[Shortcut]\nTarget=/usr/bin/files\nIcon=folder\n");
    vfs_create(desktop, "Browser.lnk", "[Shortcut]\nTarget=/usr/bin/browser\nIcon=web\n");
    vfs_create(desktop, "README.txt", "Welcome to KiKOS.11!\n\nDouble-click items to open them.\nRight-click for more options.\n");
    VfsNode *documents = vfs_mkdir(kikos_home, "Documents");
    vfs_create(documents, "notes.txt", "KiKOS Development Notes\n========================\n\n- Custom bootloader: DONE\n- Kernel with VBE: DONE\n- GUI framework: DONE\n- Antivirus: DONE\n- Filesystem: IN PROGRESS\n");
    vfs_create(documents, "todo.md", "# TODO\n\n- [x] Bootloader\n- [x] Kernel\n- [x] GUI\n- [x] Apps\n- [ ] Network stack\n- [ ] Audio\n- [ ] Package manager\n");
    VfsNode *downloads = vfs_mkdir(kikos_home, "Downloads");
    vfs_create(downloads, "kikos-update-1.1.iso", "ISO image (2.4 GB)");
    vfs_create(downloads, "drivers-pack.zip", "ZIP archive (45 MB)");
    VfsNode *pictures = vfs_mkdir(kikos_home, "Pictures");
    vfs_create(pictures, "wallpaper_aurora.png", "PNG image (1920x1080)");
    vfs_create(pictures, "screenshot_001.png", "PNG image (1024x768)");
    vfs_create(pictures, "photo.jpg", "JPEG image (4032x3024)");
    VfsNode *music = vfs_mkdir(kikos_home, "Music");
    vfs_create(music, "kikos_theme.ogg", "OGG audio (3:42)");
    vfs_create(music, "startup_chime.wav", "WAV audio (0:03)");
    VfsNode *videos = vfs_mkdir(kikos_home, "Videos");
    vfs_create(videos, "demo.mp4", "MP4 video (10:23)");

    VfsNode *lib = vfs_mkdir(g_vfs_root, "lib");
    vfs_create(lib, "libc.so", "ELF shared library");
    vfs_create(lib, "libm.so", "ELF shared library");
    vfs_create(lib, "libgui.so", "ELF shared library");
    vfs_create(lib, "libfs.so", "ELF shared library");
    vfs_create(lib, "ld-kikos.so.1", "ELF dynamic linker");

    VfsNode *media = vfs_mkdir(g_vfs_root, "media");
    vfs_create(media, ".keep", "");

    VfsNode *mnt = vfs_mkdir(g_vfs_root, "mnt");
    vfs_create(mnt, ".keep", "");

    VfsNode *opt = vfs_mkdir(g_vfs_root, "opt");
    vfs_create(opt, ".keep", "");

    VfsNode *proc = vfs_mkdir(g_vfs_root, "proc");
    vfs_create(proc, "cpuinfo", "processor\t: 0\nvendor_id\t: KiKOS\nmodel name\t: i686-compatible\ncpu MHz\t\t: 2000\ncache size\t: 64 KB\n");
    vfs_create(proc, "meminfo", "MemTotal:        262144 kB\nMemFree:         180234 kB\nMemAvailable:    210456 kB\nBuffers:           1024 kB\nCached:           45234 kB\n");
    vfs_create(proc, "version", "KiKOS.11 'Aurora' Build 2026.08\n");
    vfs_create(proc, "uptime", "1234.56 567.89\n");

    VfsNode *root_home = vfs_mkdir(g_vfs_root, "root");
    vfs_create(root_home, ".bashrc", "alias ll='ls -la'\nexport PS1='\\u@\\h:\\w\\$ '\n");

    VfsNode *run = vfs_mkdir(g_vfs_root, "run");
    vfs_create(run, "lock", "");
    vfs_create(run, "kikos.pid", "1\n");

    VfsNode *sbin = vfs_mkdir(g_vfs_root, "sbin");
    vfs_create(sbin, "fsck", "ELF binary");
    vfs_create(sbin, "mkfs", "ELF binary");
    vfs_create(sbin, "mount", "ELF binary");
    vfs_create(sbin, "reboot", "ELF binary");

    VfsNode *srv = vfs_mkdir(g_vfs_root, "srv");
    vfs_create(srv, ".keep", "");

    VfsNode *sys = vfs_mkdir(g_vfs_root, "sys");
    VfsNode *block = vfs_mkdir(sys, "block");
    vfs_create(block, "sda", "disk");
    VfsNode *sda1 = vfs_mkdir(block, "sda1");
    vfs_create(sda1, "size", "524288\n");
    vfs_create(sda1, "start", "2048\n");

    VfsNode *tmp = vfs_mkdir(g_vfs_root, "tmp");
    vfs_create(tmp, "kikos-session-1000", "session data");
    vfs_create(tmp, "cache.dat", "cached data");

    VfsNode *usr = vfs_mkdir(g_vfs_root, "usr");
    VfsNode *usr_bin = vfs_mkdir(usr, "bin");
    vfs_create(usr_bin, "browser", "ELF binary");
    vfs_create(usr_bin, "editor", "ELF binary");
    vfs_create(usr_bin, "player", "ELF binary");
    vfs_create(usr_bin, "calculator", "ELF binary");
    vfs_create(usr_bin, "settings", "ELF binary");
    VfsNode *usr_lib = vfs_mkdir(usr, "lib");
    vfs_create(usr_lib, "libwebkit.so", "ELF shared library");
    vfs_create(usr_lib, "libffmpeg.so", "ELF shared library");
    VfsNode *usr_share = vfs_mkdir(usr, "share");
    VfsNode *themes = vfs_mkdir(usr_share, "themes");
    vfs_create(themes, "aurora.theme", "[Theme]\nName=Aurora\nBackground=#0a0c14\nAccent=#00d4aa\n");
    vfs_create(themes, "sunset.theme", "[Theme]\nName=Sunset\nBackground=#1a0a14\nAccent=#ff6b35\n");
    vfs_create(themes, "ocean.theme", "[Theme]\nName=Ocean\nBackground=#0a141a\nAccent=#00aaff\n");
    vfs_create(themes, "mono.theme", "[Theme]\nName=Mono\nBackground=#0d0d0d\nAccent=#ffffff\n");
    VfsNode *icons = vfs_mkdir(usr_share, "icons");
    vfs_create(icons, "kikos.svg", "SVG icon");
    vfs_create(icons, "folder.svg", "SVG icon");
    VfsNode *applications = vfs_mkdir(usr_share, "applications");
    vfs_create(applications, "terminal.desktop", "[Desktop Entry]\nName=Terminal\nExec=/bin/kikosh\nIcon=terminal\nType=Application\n");
    vfs_create(applications, "files.desktop", "[Desktop Entry]\nName=Files\nExec=/usr/bin/files\nIcon=folder\nType=Application\n");
    vfs_create(applications, "antivirus.desktop", "[Desktop Entry]\nName=Antivirus\nExec=/usr/bin/antivirus\nIcon=shield\nType=Application\n");

    VfsNode *var = vfs_mkdir(g_vfs_root, "var");
    VfsNode *log = vfs_mkdir(var, "log");
    vfs_create(log, "kikos.log", "[INFO] Kernel initialized\n[INFO] VBE mode set\n[INFO] Drivers loaded\n[INFO] Desktop ready\n");
    vfs_create(log, "boot.log", "Boot sequence started\nMBR loaded\nVBE mode: 1024x768x32\nKernel loaded at 0x100000\n");
    VfsNode *cache = vfs_mkdir(var, "cache");
    vfs_create(cache, "font.cache", "font cache data");
    vfs_create(cache, "icon.cache", "icon cache data");
    VfsNode *lib_var = vfs_mkdir(var, "lib");
    vfs_create(lib_var, "kikos.db", "SQLite database");
    VfsNode *spool = vfs_mkdir(var, "spool");
    vfs_create(spool, ".keep", "");

    return g_vfs_root;
}

VfsNode *vfs_root(void) { return g_vfs_root; }

VfsNode *vfs_find(VfsNode *dir, const char *name) {
    if (!dir || dir->type != VFS_DIR) return 0;
    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return 0;
}

VfsNode *vfs_mkdir(VfsNode *parent, const char *name) {
    if (!parent || parent->type != VFS_DIR) return 0;
    if (vfs_find(parent, name)) return 0;
    return vfs_alloc_node(name, VFS_DIR, parent);
}

VfsNode *vfs_create(VfsNode *parent, const char *name, const char *content) {
    if (!parent || parent->type != VFS_DIR) return 0;
    if (vfs_find(parent, name)) return 0;
    VfsNode *n = vfs_alloc_node(name, VFS_FILE, parent);
    if (n && content) {
        int len = strlen(content);
        n->content = (char*)kmalloc(len + 1);
        if (n->content) {
            strcpy(n->content, content);
            n->size = len;
        }
    }
    return n;
}

VfsNode *vfs_symlink(VfsNode *parent, const char *name, const char *target) {
    if (!parent || parent->type != VFS_DIR) return 0;
    if (vfs_find(parent, name)) return 0;
    VfsNode *n = vfs_alloc_node(name, VFS_SYMLINK, parent);
    if (n && target) {
        n->link_target = (char*)kmalloc(strlen(target) + 1);
        if (n->link_target) strcpy(n->link_target, target);
    }
    return n;
}

int vfs_remove(VfsNode *parent, const char *name) {
    if (!parent || parent->type != VFS_DIR) return -1;
    for (int i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            vfs_free(parent->children[i]);
            for (int j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            return 0;
        }
    }
    return -1;
}

int vfs_rename(VfsNode *node, const char *new_name) {
    if (!node) return -1;
    strncpy(node->name, new_name, VFS_MAX_NAME - 1);
    node->modified = g_ticks;
    return 0;
}

void vfs_free(VfsNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++)
        vfs_free(node->children[i]);
    if (node->content) kfree(node->content);
    if (node->link_target) kfree(node->link_target);
}

void vfs_print_tree(VfsNode *node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) dbg("  ");
    dbg(node->name);
    if (node->type == VFS_DIR) dbg("/");
    dbg("\n");
    if (node->type == VFS_DIR) {
        for (int i = 0; i < node->child_count; i++)
            vfs_print_tree(node->children[i], depth + 1);
    }
}

char *vfs_get_path(VfsNode *node, char *buf, int buflen) {
    if (!node || !buf || buflen < 2) return 0;
    char tmp[VFS_MAX_PATH];
    int pos = 0;
    VfsNode *cur = node;
    while (cur && cur != cur->parent) {
        int len = strlen(cur->name);
        if (pos + len + 1 >= VFS_MAX_PATH) break;
        if (pos > 0) tmp[pos++] = '/';
        strcpy(tmp + pos, cur->name);
        pos += len;
        cur = cur->parent;
    }
    if (pos == 0) {
        buf[0] = '/'; buf[1] = 0;
        return buf;
    }
    for (int i = 0; i < pos; i++)
        buf[i] = tmp[pos - 1 - i];
    buf[pos] = 0;
    return buf;
}

VfsNode *vfs_resolve_path(const char *path) {
    if (!path || *path != '/') return 0;
    VfsNode *cur = g_vfs_root;
    const char *p = path + 1;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != '/') p++;
        int len = p - start;
        char name[VFS_MAX_NAME];
        if (len >= VFS_MAX_NAME) return 0;
        strncpy(name, start, len);
        name[len] = 0;
        cur = vfs_find(cur, name);
        if (!cur) return 0;
    }
    return cur;
}

int vfs_read(const char *path, char *buf, int buflen) {
    VfsNode *n = vfs_resolve_path(path);
    if (!n || n->type != VFS_FILE) return -1;
    if (!n->content) return 0;
    int len = n->size;
    if (len > buflen - 1) len = buflen - 1;
    memcpy(buf, n->content, len);
    buf[len] = 0;
    return len;
}

int vfs_write(const char *path, const char *data, int len) {
    VfsNode *n = vfs_resolve_path(path);
    if (!n) {
        char dir_path[VFS_MAX_PATH];
        strcpy(dir_path, path);
        char *last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = 0;
            VfsNode *dir = vfs_resolve_path(dir_path);
            if (dir && dir->type == VFS_DIR) {
                n = vfs_create(dir, last_slash + 1, "");
            }
        }
    }
    if (!n || n->type != VFS_FILE) return -1;
    if (n->content) kfree(n->content);
    n->content = (char*)kmalloc(len + 1);
    if (!n->content) return -1;
    memcpy(n->content, data, len);
    n->content[len] = 0;
    n->size = len;
    n->modified = g_ticks;
    return len;
}

int vfs_append(const char *path, const char *data, int len) {
    VfsNode *n = vfs_resolve_path(path);
    if (!n) return vfs_write(path, data, len);
    if (n->type != VFS_FILE) return -1;
    int old_len = n->size;
    char *new_content = (char*)kmalloc(old_len + len + 1);
    if (!new_content) return -1;
    if (n->content) {
        memcpy(new_content, n->content, old_len);
        kfree(n->content);
    }
    memcpy(new_content + old_len, data, len);
    new_content[old_len + len] = 0;
    n->content = new_content;
    n->size = old_len + len;
    n->modified = g_ticks;
    return len;
}

int vfs_mkdir_p(const char *path) {
    char tmp[VFS_MAX_PATH];
    strncpy(tmp, path, VFS_MAX_PATH - 1);
    tmp[VFS_MAX_PATH - 1] = 0;
    
    VfsNode *cur = g_vfs_root;
    char *p = tmp + 1;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        char *start = p;
        while (*p && *p != '/') p++;
        int len = p - start;
        char name[VFS_MAX_NAME];
        if (len >= VFS_MAX_NAME) return -1;
        strncpy(name, start, len);
        name[len] = 0;
        
        VfsNode *next = vfs_find(cur, name);
        if (!next) {
            next = vfs_mkdir(cur, name);
            if (!next) return -1;
        }
        if (next->type != VFS_DIR) return -1;
        cur = next;
    }
    return 0;
}

int vfs_rm_r(const char *path) {
    VfsNode *n = vfs_resolve_path(path);
    if (!n) return -1;
    char name[VFS_MAX_NAME];
    strncpy(name, n->name, VFS_MAX_NAME - 1);
    VfsNode *parent = n->parent;
    if (!parent) return -1;
    vfs_free(n);
    for (int i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            for (int j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            return 0;
        }
    }
    return -1;
}

int vfs_cp(const char *src, const char *dst) {
    VfsNode *sn = vfs_resolve_path(src);
    if (!sn || sn->type != VFS_FILE) return -1;
    
    char dst_dir[VFS_MAX_PATH];
    strncpy(dst_dir, dst, VFS_MAX_PATH - 1);
    dst_dir[VFS_MAX_PATH - 1] = 0;
    char *last_slash = strrchr(dst_dir, '/');
    if (!last_slash) return -1;
    *last_slash = 0;
    if (dst_dir[0] == 0) strcpy(dst_dir, "/");
    
    VfsNode *d = vfs_resolve_path(dst_dir);
    if (!d || d->type != VFS_DIR) {
        if (vfs_mkdir_p(dst_dir) != 0) return -1;
        d = vfs_resolve_path(dst_dir);
        if (!d) return -1;
    }
    
    VfsNode *nn = vfs_create(d, last_slash + 1, "");
    if (!nn) return -1;
    if (sn->content) {
        nn->content = (char*)kmalloc(sn->size + 1);
        if (!nn->content) return -1;
        memcpy(nn->content, sn->content, sn->size);
        nn->content[sn->size] = 0;
        nn->size = sn->size;
    }
    return 0;
}

int vfs_mv(const char *src, const char *dst) {
    if (vfs_cp(src, dst) != 0) return -1;
    return vfs_rm_r(src);
}

int vfs_stat(const char *path, VfsStat *st) {
    VfsNode *n = vfs_resolve_path(path);
    if (!n) return -1;
    st->type = n->type;
    st->mode = n->mode;
    st->uid = n->uid;
    st->gid = n->gid;
    st->size = n->size;
    st->nlink = (n->type == VFS_DIR) ? 2 + n->child_count : 1;
    st->created = n->created;
    st->modified = n->modified;
    return 0;
}

char *vfs_basename(const char *path, char *buf, int buflen) {
    const char *p = path + strlen(path) - 1;
    while (p > path && *p == '/') p--;
    const char *end = p;
    while (p > path && *p != '/') p--;
    if (*p == '/') p++;
    int len = end - p + 1;
    if (len >= buflen) len = buflen - 1;
    strncpy(buf, p, len);
    buf[len] = 0;
    return buf;
}

char *vfs_dirname(const char *path, char *buf, int buflen) {
    const char *p = path + strlen(path) - 1;
    while (p > path && *p == '/') p--;
    while (p > path && *p != '/') p--;
    if (p == path) {
        buf[0] = '/'; buf[1] = 0;
        return buf;
    }
    int len = p - path;
    if (len >= buflen) len = buflen - 1;
    strncpy(buf, path, len);
    buf[len] = 0;
    return buf;
}