#include "antivirus.h"
#include "lib.h"
#include "vfs.h"
#include "timer.h"
#include "com1.h"
#include <stdio.h>

Antivirus g_antivirus;

static const AvSignature builtin_sigs[] = {
    {"EICAR-Test", "X5O!P%@AP[4\\PZX54(P^)7CC)", AV_THREAT_VIRUS, 10},
    {"Trojan.Memdump", "trojan.memdump", AV_THREAT_TROJAN, 9},
    {"Worm.KernelMod", "kernel.mod.inject", AV_THREAT_WORM, 8},
    {"Adware.CacheLeak", "cache.leak.adw", AV_THREAT_ADWARE, 5},
    {"Spy.ReadKeyLog", "keylog.hook", AV_THREAT_SPYWARE, 7},
    {"Ransom.Crypt", "cryptolocker.enc", AV_THREAT_RANSOMWARE, 10},
    {"Rootkit.Hook", "kernel.hook.root", AV_THREAT_ROOTKIT, 9},
    {"Trojan.Bank", "banking.steal", AV_THREAT_TROJAN, 9},
    {"Worm.Conficker", "conficker.net", AV_THREAT_WORM, 8},
    {"Exploit.Eternal", "eternalblue.smb", AV_THREAT_VIRUS, 10},
};

int av_init(void) {
    memset(&g_antivirus, 0, sizeof(Antivirus));
    g_antivirus.enabled = 1;
    g_antivirus.realtime_enabled = 1;
    g_antivirus.heuristic_enabled = 1;
    g_antivirus.auto_quarantine = 1;

    int n = sizeof(builtin_sigs) / sizeof(AvSignature);
    for (int i = 0; i < n && i < AV_MAX_SIGNATURES; i++)
        g_antivirus.signatures[g_antivirus.sig_count++] = builtin_sigs[i];

    vfs_mkdir_p("/var/quarantine");

    dbg("[antivirus] initialized\n");
    return 0;
}

int av_load_signatures(void) { return 0; }
int av_update_signatures(void) { g_antivirus.last_update = g_ticks; return 0; }

static int check_sig(const char *content, int len, AvThreatType *type, char *name) {
    for (int i = 0; i < g_antivirus.sig_count; i++) {
        int slen = strlen(g_antivirus.signatures[i].signature);
        if (slen == 0 || len < slen) continue;
        for (int j = 0; j <= len - slen; j++) {
            if (memcmp(content + j, g_antivirus.signatures[i].signature, slen) == 0) {
                *type = g_antivirus.signatures[i].type;
                strncpy(name, g_antivirus.signatures[i].name, AV_MAX_NAME - 1);
                return 1;
            }
        }
    }
    return 0;
}

int av_scan_file(const char *path, AvThreatType *type, char *threat_name) {
    char buf[2048];
    int len = vfs_read(path, buf, sizeof(buf) - 1);
    if (len < 0) return -1;

    g_antivirus.files_scanned++;

    if (check_sig(buf, len, type, threat_name)) {
        g_antivirus.threats_found++;
        return 1;
    }
    *type = AV_THREAT_NONE;
    return 0;
}

int av_scan_directory(const char *path, int recursive) {
    VfsNode *dir = vfs_resolve_path(path);
    if (!dir || dir->type != VFS_DIR) return -1;

    for (int i = 0; i < dir->child_count; i++) {
        VfsNode *n = dir->children[i];
        char full[AV_MAX_PATH];
        vfs_get_path(n, full, sizeof(full));

        if (n->type == VFS_FILE) {
            AvThreatType type;
            char name[AV_MAX_NAME];
            if (av_scan_file(full, &type, name) > 0 && g_antivirus.auto_quarantine)
                av_quarantine(full, type, name);
        } else if (recursive && n->type == VFS_DIR) {
            av_scan_directory(full, 1);
        }
    }
    return 0;
}

int av_quarantine(const char *path, AvThreatType type, const char *threat_name) {
    if (g_antivirus.quarantine_count >= AV_MAX_QUARANTINE) return -1;

    char buf[2048];
    int len = vfs_read(path, buf, sizeof(buf));
    if (len < 0) return -1;

    AvQuarantine *q = &g_antivirus.quarantine[g_antivirus.quarantine_count++];
    strncpy(q->original_path, path, AV_MAX_PATH - 1);
    snprintf(q->quarantine_path, AV_MAX_PATH, "/var/quarantine/q_%d", g_ticks);
    q->type = type;
    strncpy(q->threat_name, threat_name, AV_MAX_NAME - 1);
    q->detected_time = g_ticks;
    q->file_size = len;

    vfs_write(q->quarantine_path, buf, len);
    vfs_rm_r(path);
    return 0;
}

int av_restore(const char *qpath) {
    for (int i = 0; i < g_antivirus.quarantine_count; i++) {
        if (strcmp(g_antivirus.quarantine[i].quarantine_path, qpath) == 0) {
            AvQuarantine *q = &g_antivirus.quarantine[i];
            char buf[2048];
            int len = vfs_read(qpath, buf, sizeof(buf));
            if (len < 0) return -1;
            vfs_write(q->original_path, buf, len);
            vfs_rm_r(qpath);
            for (int j = i; j < g_antivirus.quarantine_count - 1; j++)
                g_antivirus.quarantine[j] = g_antivirus.quarantine[j + 1];
            g_antivirus.quarantine_count--;
            return 0;
        }
    }
    return -1;
}

int av_delete_quarantine(const char *qpath) {
    for (int i = 0; i < g_antivirus.quarantine_count; i++) {
        if (strcmp(g_antivirus.quarantine[i].quarantine_path, qpath) == 0) {
            vfs_rm_r(qpath);
            for (int j = i; j < g_antivirus.quarantine_count - 1; j++)
                g_antivirus.quarantine[j] = g_antivirus.quarantine[j + 1];
            g_antivirus.quarantine_count--;
            return 0;
        }
    }
    return -1;
}

int av_list_quarantine(AvQuarantine **list, int max) {
    int count = 0;
    for (int i = 0; i < g_antivirus.quarantine_count && count < max; i++)
        list[count++] = &g_antivirus.quarantine[i];
    return count;
}

int av_real_time_check(const char *path) {
    if (!g_antivirus.realtime_enabled) return 0;
    AvThreatType type;
    char name[AV_MAX_NAME];
    return av_scan_file(path, &type, name);
}

int av_get_status(char *buf, int buflen) {
    return snprintf(buf, buflen,
        "Antivirus: %s\nReal-time: %s\nSignatures: %d\nQuarantined: %d\nScanned: %d\nThreats: %d\n",
        g_antivirus.enabled ? "ON" : "OFF",
        g_antivirus.realtime_enabled ? "ON" : "OFF",
        g_antivirus.sig_count,
        g_antivirus.quarantine_count,
        g_antivirus.files_scanned,
        g_antivirus.threats_found);
}

int av_update_db(void) { return av_update_signatures(); }