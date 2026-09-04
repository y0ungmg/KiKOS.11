#include "syscall.h"
#include "process.h"
#include "vfs.h"
#include "lib.h"
#include "timer.h"
#include "mm.h"
#include "com1.h"
#include "rtc.h"

static int (*syscall_table[64])(syscall_regs_t *);

int sys_exit_handler(syscall_regs_t *r) {
    proc_exit(SYSCALL_ARG0(r));
    return 0;
}

int sys_fork_handler(syscall_regs_t *r) {
    (void)r;
    return sys_fork();
}

int sys_execve_handler(syscall_regs_t *r) {
    return sys_execve((const char *)SYSCALL_ARG0(r), (char *const *)SYSCALL_ARG1(r), (char *const *)SYSCALL_ARG2(r));
}

int sys_waitpid_handler(syscall_regs_t *r) {
    return sys_waitpid(SYSCALL_ARG0(r), (int *)SYSCALL_ARG1(r), SYSCALL_ARG2(r));
}

int sys_getpid_handler(syscall_regs_t *r) {
    (void)r;
    return sys_getpid();
}

int sys_getppid_handler(syscall_regs_t *r) {
    (void)r;
    return sys_getppid();
}

int sys_yield_handler(syscall_regs_t *r) {
    (void)r;
    yield();
    return 0;
}

int sys_sleep_handler(syscall_regs_t *r) {
    return sys_sleep(SYSCALL_ARG0(r));
}

int sys_kill_handler(syscall_regs_t *r) {
    return sys_kill(SYSCALL_ARG0(r), SYSCALL_ARG1(r));
}

int sys_open_handler(syscall_regs_t *r) {
    const char *path = (const char *)SYSCALL_ARG0(r);
    int flags = SYSCALL_ARG1(r);
    int mode = SYSCALL_ARG2(r);
    (void)flags; (void)mode;
    VfsNode *n = vfs_resolve_path(path);
    if (!n) return -1;
    return (int)n;
}

int sys_close_handler(syscall_regs_t *r) {
    int fd = SYSCALL_ARG0(r);
    if (fd < 0 || fd >= 64) return -1;
    if (current_process && current_process->fd_table[fd]) {
        current_process->fd_table[fd] = 0;
        return 0;
    }
    return -1;
}

int sys_read_handler(syscall_regs_t *r) {
    int fd = SYSCALL_ARG0(r);
    void *buf = (void *)SYSCALL_ARG1(r);
    int count = SYSCALL_ARG2(r);
    if (fd < 0 || fd >= 64) return -1;
    VfsNode *n = (VfsNode *)current_process->fd_table[fd];
    if (!n || n->type != VFS_FILE) return -1;
    return vfs_read((const char *)n->name, buf, count);
}

int sys_write_handler(syscall_regs_t *r) {
    int fd = SYSCALL_ARG0(r);
    const void *buf = (const void *)SYSCALL_ARG1(r);
    int count = SYSCALL_ARG2(r);
    if (fd < 0 || fd >= 64) return -1;
    VfsNode *n = (VfsNode *)current_process->fd_table[fd];
    if (!n || n->type != VFS_FILE) return -1;
    return vfs_write((const char *)n->name, buf, count);
}

int sys_lseek_handler(syscall_regs_t *r) {
    (void)r;
    return -1;
}

int sys_stat_handler(syscall_regs_t *r) {
    const char *path = (const char *)SYSCALL_ARG0(r);
    VfsStat *st = (VfsStat *)SYSCALL_ARG1(r);
    return vfs_stat(path, st);
}

int sys_gettime_handler(syscall_regs_t *r) {
    (void)r;
    rtc_poll();
    return (g_rtc.hour * 3600 + g_rtc.min * 60 + g_rtc.sec) * 1000 + g_ticks % 1000;
}

int sys_getuid_handler(syscall_regs_t *r) {
    (void)r;
    return 0;
}

int sys_getgid_handler(syscall_regs_t *r) {
    (void)r;
    return 0;
}

int sys_chdir_handler(syscall_regs_t *r) {
    const char *path = (const char *)SYSCALL_ARG0(r);
    VfsNode *n = vfs_resolve_path(path);
    if (!n || n->type != VFS_DIR) return -1;
    return 0;
}

int sys_getcwd_handler(syscall_regs_t *r) {
    char *buf = (char *)SYSCALL_ARG0(r);
    int size = SYSCALL_ARG1(r);
    if (!current_process) return -1;
    vfs_get_path((VfsNode *)current_process->fd_table[0], buf, size);
    return 0;
}

int sys_mkdir_handler(syscall_regs_t *r) {
    const char *path = (const char *)SYSCALL_ARG0(r);
    int mode = SYSCALL_ARG1(r);
    (void)mode;
    VfsNode *n = vfs_mkdir(vfs_resolve_path(path), "");
    return n ? 0 : -1;
}

int sys_unlink_handler(syscall_regs_t *r) {
    const char *path = (const char *)SYSCALL_ARG0(r);
    char dir[256], name[64];
    strncpy(dir, path, 255);
    char *last = strrchr(dir, '/');
    if (!last) return -1;
    *last = 0;
    strcpy(name, last + 1);
    VfsNode *d = vfs_resolve_path(dir);
    if (!d) return -1;
    return vfs_remove(d, name);
}

int sys_rename_handler(syscall_regs_t *r) {
    const char *old = (const char *)SYSCALL_ARG0(r);
    const char *new = (const char *)SYSCALL_ARG1(r);
    char old_dir[256], old_name[64], new_dir[256], new_name[64];
    strncpy(old_dir, old, 255);
    char *last = strrchr(old_dir, '/');
    if (!last) return -1;
    *last = 0;
    strcpy(old_name, last + 1);
    strncpy(new_dir, new, 255);
    last = strrchr(new_dir, '/');
    if (!last) return -1;
    *last = 0;
    strcpy(new_name, last + 1);
    VfsNode *od = vfs_resolve_path(old_dir);
    VfsNode *nd = vfs_resolve_path(new_dir);
    if (!od || !nd) return -1;
    return vfs_mv(old, new);
}

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));
    syscall_table[SYSCALL_EXIT] = sys_exit_handler;
    syscall_table[SYSCALL_FORK] = sys_fork_handler;
    syscall_table[SYSCALL_EXECVE] = sys_execve_handler;
    syscall_table[SYSCALL_WAITPID] = sys_waitpid_handler;
    syscall_table[SYSCALL_GETPID] = sys_getpid_handler;
    syscall_table[SYSCALL_GETPPID] = sys_getppid_handler;
    syscall_table[SYSCALL_YIELD] = sys_yield_handler;
    syscall_table[SYSCALL_SLEEP] = sys_sleep_handler;
    syscall_table[SYSCALL_KILL] = sys_kill_handler;
    syscall_table[SYSCALL_OPEN] = sys_open_handler;
    syscall_table[SYSCALL_CLOSE] = sys_close_handler;
    syscall_table[SYSCALL_READ] = sys_read_handler;
    syscall_table[SYSCALL_WRITE] = sys_write_handler;
    syscall_table[SYSCALL_LSEEK] = sys_lseek_handler;
    syscall_table[SYSCALL_STAT] = sys_stat_handler;
    syscall_table[SYSCALL_GETTIME] = sys_gettime_handler;
    syscall_table[SYSCALL_GETUID] = sys_getuid_handler;
    syscall_table[SYSCALL_GETGID] = sys_getgid_handler;
    syscall_table[SYSCALL_CHDIR] = sys_chdir_handler;
    syscall_table[SYSCALL_GETCWD] = sys_getcwd_handler;
    syscall_table[SYSCALL_MKDIR] = sys_mkdir_handler;
    syscall_table[SYSCALL_UNLINK] = sys_unlink_handler;
    syscall_table[SYSCALL_RENAME] = sys_rename_handler;
}

int syscall_handler(syscall_regs_t *r) {
    if (r->syscall_num >= 0 && r->syscall_num < 64 && syscall_table[r->syscall_num]) {
        return syscall_table[r->syscall_num](r);
    }
    return -1;
}