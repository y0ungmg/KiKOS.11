#pragma once

#include "types.h"

#define FW_MAX_RULES     256
#define FW_MAX_CHAINS    8
#define FW_MAX_LOG       1024
#define FW_MAX_IP_LEN    16

typedef enum {
    FW_ACTION_ACCEPT,
    FW_ACTION_DROP,
    FW_ACTION_REJECT,
    FW_ACTION_LOG
} FwAction;

typedef enum {
    FW_PROTO_TCP = 6,
    FW_PROTO_UDP = 17,
    FW_PROTO_ICMP = 1,
    FW_PROTO_ANY = 255
} FwProto;

typedef enum {
    FW_DIR_IN,
    FW_DIR_OUT,
    FW_DIR_FWD
} FwDir;

typedef struct {
    char name[32];
    FwAction policy;
    int rule_count;
} FwChain;

typedef struct {
    int enabled;
    FwChain chains[FW_MAX_CHAINS];
    int chain_count;
} FwConfig;

typedef struct {
    int enabled;
    FwAction action;
    FwProto proto;
    FwDir dir;
    char src_ip[FW_MAX_IP_LEN];
    int src_port;
    char dst_ip[FW_MAX_IP_LEN];
    int dst_port;
    char comment[64];
    u32 packets;
    u32 bytes;
    u32 created;
} FwRule;

typedef struct {
    int enabled;
    FwChain chains[FW_MAX_CHAINS];
    int chain_count;
    FwRule rules[FW_MAX_RULES];
    int rule_count;
    int log_count;
    struct {
        u32 time;
        int rule_idx;
        char src_ip[FW_MAX_IP_LEN];
        char dst_ip[FW_MAX_IP_LEN];
        int src_port;
        int dst_port;
        FwProto proto;
        FwAction action;
    } logs[FW_MAX_LOG];
} Firewall;

extern Firewall g_firewall;

int fw_init(void);
int fw_enable(void);
int fw_disable(void);
int fw_add_rule(FwRule *rule);
int fw_remove_rule(int idx);
int fw_list_rules(FwRule **list, int max);
int fw_check_packet(const char *src_ip, int src_port, const char *dst_ip, int dst_port, FwProto proto, FwDir dir);
int fw_log_packet(int rule_idx, const char *src_ip, const char *dst_ip, int src_port, int dst_port, FwProto proto, FwAction action);
int fw_get_logs(void **logs, int *count);
int fw_clear_logs(void);
int fw_set_default_policy(FwChain *chain, FwAction policy);
int fw_save_config(const char *path);
int fw_load_config(const char *path);
int fw_status(char *buf, int buflen);