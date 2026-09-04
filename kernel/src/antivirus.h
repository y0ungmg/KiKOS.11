#pragma once

#include "types.h"
#include "vfs.h"

#define AV_MAX_SIGNATURES  64
#define AV_MAX_QUARANTINE  32
#define AV_MAX_PATH        128
#define AV_MAX_NAME        48

typedef enum {
    AV_THREAT_NONE,
    AV_THREAT_VIRUS,
    AV_THREAT_TROJAN,
    AV_THREAT_WORM,
    AV_THREAT_RANSOMWARE,
    AV_THREAT_SPYWARE,
    AV_THREAT_ADWARE,
    AV_THREAT_ROOTKIT,
    AV_THREAT_SUSPICIOUS
} AvThreatType;

typedef enum {
    AV_ACTION_QUARANTINE,
    AV_ACTION_DELETE,
    AV_ACTION_IGNORE,
    AV_ACTION_ALERT
} AvAction;

typedef struct {
    char name[AV_MAX_NAME];
    char signature[64];
    AvThreatType type;
    int severity;
} AvSignature;

typedef struct {
    char original_path[AV_MAX_PATH];
    char quarantine_path[AV_MAX_PATH];
    AvThreatType type;
    char threat_name[AV_MAX_NAME];
    u32 detected_time;
    u32 file_size;
} AvQuarantine;

typedef struct {
    int enabled;
    int realtime_enabled;
    int heuristic_enabled;
    int auto_quarantine;
    AvSignature signatures[AV_MAX_SIGNATURES];
    int sig_count;
    AvQuarantine quarantine[AV_MAX_QUARANTINE];
    int quarantine_count;
    u32 last_scan_time;
    u32 files_scanned;
    u32 threats_found;
    u32 last_update;
} Antivirus;

extern Antivirus g_antivirus;

int av_init(void);
int av_load_signatures(void);
int av_update_signatures(void);
int av_scan_file(const char *path, AvThreatType *type, char *threat_name);
int av_scan_directory(const char *path, int recursive);
int av_quarantine(const char *path, AvThreatType type, const char *threat_name);
int av_restore(const char *quarantine_path);
int av_delete_quarantine(const char *quarantine_path);
int av_list_quarantine(AvQuarantine **list, int max);
int av_real_time_check(const char *path);
int av_get_status(char *buf, int buflen);
int av_update_db(void);