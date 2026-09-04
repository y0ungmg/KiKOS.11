#include "firewall.h"
#include "lib.h"
#include "timer.h"
#include "com1.h"
#include <stdio.h>

Firewall g_firewall;

int fw_init(void) {
    memset(&g_firewall, 0, sizeof(Firewall));
    g_firewall.enabled = 1;
    
    // Default chains
    strcpy(g_firewall.chains[0].name, "INPUT");
    g_firewall.chains[0].policy = FW_ACTION_DROP;
    g_firewall.chains[0].rule_count = 0;
    
    strcpy(g_firewall.chains[1].name, "OUTPUT");
    g_firewall.chains[1].policy = FW_ACTION_ACCEPT;
    g_firewall.chains[1].rule_count = 0;
    
    strcpy(g_firewall.chains[2].name, "FORWARD");
    g_firewall.chains[2].policy = FW_ACTION_DROP;
    g_firewall.chains[2].rule_count = 0;
    
    g_firewall.chain_count = 3;
    
    // Default rules: allow loopback, established connections
    FwRule rule;
    memset(&rule, 0, sizeof(FwRule));
    rule.enabled = 1;
    rule.action = FW_ACTION_ACCEPT;
    rule.proto = FW_PROTO_ANY;
    rule.dir = FW_DIR_IN;
    strcpy(rule.src_ip, "127.0.0.1");
    strcpy(rule.comment, "Loopback");
    fw_add_rule(&rule);
    
    rule.dir = FW_DIR_OUT;
    fw_add_rule(&rule);
    
    // Allow established/related
    rule.dir = FW_DIR_IN;
    strcpy(rule.src_ip, "0.0.0.0");
    strcpy(rule.dst_ip, "0.0.0.0");
    rule.src_port = 0;
    rule.dst_port = 0;
    strcpy(rule.comment, "Established connections");
    fw_add_rule(&rule);
    
    rule.dir = FW_DIR_OUT;
    fw_add_rule(&rule);
    
    // Allow DHCP
    rule.proto = FW_PROTO_UDP;
    rule.dir = FW_DIR_IN;
    rule.src_port = 67;
    rule.dst_port = 68;
    strcpy(rule.comment, "DHCP");
    fw_add_rule(&rule);
    
    // Allow DNS
    rule.proto = FW_PROTO_UDP;
    rule.dir = FW_DIR_OUT;
    rule.src_port = 0;
    rule.dst_port = 53;
    strcpy(rule.comment, "DNS");
    fw_add_rule(&rule);
    
    // Allow HTTP/HTTPS
    rule.proto = FW_PROTO_TCP;
    rule.dir = FW_DIR_OUT;
    rule.dst_port = 80;
    strcpy(rule.comment, "HTTP");
    fw_add_rule(&rule);
    
    rule.dst_port = 443;
    strcpy(rule.comment, "HTTPS");
    fw_add_rule(&rule);
    
    dbg("[firewall] initialized with default rules\n");
    return 0;
}

int fw_enable(void) {
    g_firewall.enabled = 1;
    dbg("[firewall] enabled\n");
    return 0;
}

int fw_disable(void) {
    g_firewall.enabled = 0;
    dbg("[firewall] disabled\n");
    return 0;
}

int fw_add_rule(FwRule *rule) {
    if (g_firewall.rule_count >= FW_MAX_RULES) return -1;
    rule->created = g_ticks;
    rule->packets = 0;
    rule->bytes = 0;
    g_firewall.rules[g_firewall.rule_count++] = *rule;
    dbg("[firewall] added rule: ");
    dbg(rule->comment);
    dbg("\n");
    return 0;
}

int fw_remove_rule(int idx) {
    if (idx < 0 || idx >= g_firewall.rule_count) return -1;
    for (int i = idx; i < g_firewall.rule_count - 1; i++)
        g_firewall.rules[i] = g_firewall.rules[i + 1];
    g_firewall.rule_count--;
    return 0;
}

int fw_list_rules(FwRule **list, int max) {
    int count = 0;
    for (int i = 0; i < g_firewall.rule_count && count < max; i++) {
        if (g_firewall.rules[i].enabled)
            list[count++] = &g_firewall.rules[i];
    }
    return count;
}

int fw_match_rule(FwRule *rule, const char *src_ip, int src_port, const char *dst_ip, int dst_port, FwProto proto, FwDir dir) {
    if (!rule->enabled) return 0;
    if (rule->proto != FW_PROTO_ANY && rule->proto != proto) return 0;
    if (rule->dir != dir) return 0;
    if (rule->src_port && rule->src_port != src_port) return 0;
    if (rule->dst_port && rule->dst_port != dst_port) return 0;
    if (strcmp(rule->src_ip, "0.0.0.0") != 0 && strcmp(rule->src_ip, src_ip) != 0) return 0;
    if (strcmp(rule->dst_ip, "0.0.0.0") != 0 && strcmp(rule->dst_ip, dst_ip) != 0) return 0;
    return 1;
}

int fw_check_packet(const char *src_ip, int src_port, const char *dst_ip, int dst_port, FwProto proto, FwDir dir) {
    if (!g_firewall.enabled) return FW_ACTION_ACCEPT;
    
    // Check rules in order
    for (int i = 0; i < g_firewall.rule_count; i++) {
        if (fw_match_rule(&g_firewall.rules[i], src_ip, src_port, dst_ip, dst_port, proto, dir)) {
            g_firewall.rules[i].packets++;
            g_firewall.rules[i].bytes += 64; // simulated
            fw_log_packet(i, src_ip, dst_ip, src_port, dst_port, proto, g_firewall.rules[i].action);
            return g_firewall.rules[i].action;
        }
    }
    
    // Default policy based on chain
    if (dir == FW_DIR_IN) return g_firewall.chains[0].policy;
    else if (dir == FW_DIR_OUT) return g_firewall.chains[1].policy;
    else return g_firewall.chains[2].policy;
}

int fw_log_packet(int rule_idx, const char *src_ip, const char *dst_ip, int src_port, int dst_port, FwProto proto, FwAction action) {
    if (g_firewall.log_count >= FW_MAX_LOG) return 0;
    g_firewall.logs[g_firewall.log_count].time = g_ticks;
    g_firewall.logs[g_firewall.log_count].rule_idx = rule_idx;
    strncpy(g_firewall.logs[g_firewall.log_count].src_ip, src_ip, FW_MAX_IP_LEN - 1);
    strncpy(g_firewall.logs[g_firewall.log_count].dst_ip, dst_ip, FW_MAX_IP_LEN - 1);
    g_firewall.logs[g_firewall.log_count].src_port = src_port;
    g_firewall.logs[g_firewall.log_count].dst_port = dst_port;
    g_firewall.logs[g_firewall.log_count].proto = proto;
    g_firewall.logs[g_firewall.log_count].action = action;
    g_firewall.log_count++;
    return 0;
}

int fw_get_logs(void **logs, int *count) {
    *logs = g_firewall.logs;
    *count = g_firewall.log_count;
    return 0;
}

int fw_clear_logs(void) {
    g_firewall.log_count = 0;
    return 0;
}

int fw_set_default_policy(FwChain *chain, FwAction policy) {
    chain->policy = policy;
    return 0;
}

int fw_save_config(const char *path) {
    (void)path;
    // Would serialize to JSON
    return 0;
}

int fw_load_config(const char *path) {
    (void)path;
    // Would parse JSON
    return 0;
}

int fw_status(char *buf, int buflen) {
    int len = 0;
    len += snprintf(buf + len, buflen - len, "Firewall: %s\n", g_firewall.enabled ? "ENABLED" : "DISABLED");
    len += snprintf(buf + len, buflen - len, "Rules: %d\n", g_firewall.rule_count);
    len += snprintf(buf + len, buflen - len, "Logs: %d\n", g_firewall.log_count);
    len += snprintf(buf + len, buflen - len, "Chains: %d\n", g_firewall.chain_count);
    for (int i = 0; i < g_firewall.chain_count; i++) {
        len += snprintf(buf + len, buflen - len, "  %s: %s\n", 
            g_firewall.chains[i].name,
            g_firewall.chains[i].policy == FW_ACTION_ACCEPT ? "ACCEPT" :
            g_firewall.chains[i].policy == FW_ACTION_DROP ? "DROP" :
            g_firewall.chains[i].policy == FW_ACTION_REJECT ? "REJECT" : "LOG");
    }
    return len;
}