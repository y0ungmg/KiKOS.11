#include "net.h"
#include "lib.h"
#include "timer.h"
#include "mm.h"
#include "com1.h"

socket_t sockets[MAX_SOCKETS] = {0};
uint8_t my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
uint32_t my_ip = 0x0A00020F;
uint32_t gateway_ip = 0x0A000201;
uint32_t subnet_mask = 0xFFFFFF00;
uint32_t dns_server = 0x01010101;
uint16_t ip_id = 1;
uint16_t tcp_port_base = 1024;

static uint16_t arp_cache_count = 0;
static struct {
    uint32_t ip;
    uint8_t mac[6];
    uint32_t expires;
} arp_cache[64];

static uint16_t tcp_seq = 1;

void net_init(void) {
    dbg("[NET] Initializing network stack\n");
    dbg("[NET] MAC: ");
    for (int i = 0; i < 6; i++) {
        char buf[4];
        utoa_dec(my_mac[i], buf);
        dbg(buf);
        if (i < 5) dbg(":");
    }
    dbg(" IP: ");
    char buf[16];
    utoa_dec(my_ip & 0xFF, buf); dbg(buf); dbg(".");
    utoa_dec((my_ip >> 8) & 0xFF, buf); dbg(buf); dbg(".");
    utoa_dec((my_ip >> 16) & 0xFF, buf); dbg(buf); dbg(".");
    utoa_dec((my_ip >> 24) & 0xFF, buf); dbg(buf);
    dbg("\n");

    for (int i = 0; i < MAX_SOCKETS; i++) {
        sockets[i].state = 0;
    }
}

void net_rx(uint8_t *packet, int len) {
    if ((unsigned int)len < sizeof(eth_header_t)) return;
    eth_header_t *eth = (eth_header_t *)packet;

    if (memcmp(eth->dst, my_mac, 6) != 0 &&
        memcmp(eth->dst, (uint8_t[]){0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, 6) != 0) {
        return;
    }

    uint16_t type = ntohs(eth->type);
    uint8_t *payload = packet + sizeof(eth_header_t);
    int payload_len = len - sizeof(eth_header_t);

    switch (type) {
        case ETH_P_ARP:
            net_handle_arp(payload, payload_len, eth->src);
            break;
        case ETH_P_IP:
            net_handle_ip(payload, payload_len, eth->src);
            break;
        default:
            break;
    }
}

void net_tx(uint8_t *packet, int len) {
    // Would send via network driver
    (void)packet; (void)len;
}



void net_handle_arp(uint8_t *data, int len, uint8_t *src_mac) {
    (void)src_mac;
    if (len < (int)sizeof(arp_packet_t)) return;
    arp_packet_t *arp = (arp_packet_t *)data;

    if (ntohs(arp->opcode) == ARP_OP_REQUEST) {
        uint32_t target_ip = *(uint32_t *)arp->target_ip;
        if (target_ip == my_ip) {
            arp_packet_t reply;
            reply.hw_type[0] = 0; reply.hw_type[1] = 1;
            reply.proto_type[0] = 0x08; reply.proto_type[1] = 0;
            reply.hw_size = 6;
            reply.proto_size = 4;
            reply.opcode = htons(ARP_OP_REPLY);
            memcpy(reply.sender_mac, my_mac, 6);
            memcpy(reply.sender_ip, (uint8_t *)&my_ip, 4);
            memcpy(reply.target_mac, arp->sender_mac, 6);
            memcpy(reply.target_ip, arp->sender_ip, 4);

            eth_header_t eth;
            memcpy(eth.dst, arp->sender_mac, 6);
            memcpy(eth.src, my_mac, 6);
            eth.type = htons(ETH_P_ARP);

            uint8_t pkt[sizeof(eth_header_t) + sizeof(arp_packet_t)];
            memcpy(pkt, &eth, sizeof(eth_header_t));
            memcpy(pkt + sizeof(eth_header_t), &reply, sizeof(arp_packet_t));
            net_tx(pkt, sizeof(pkt));
        }
    } else if (ntohs(arp->opcode) == ARP_OP_REPLY) {
        uint32_t ip = *(uint32_t *)arp->sender_ip;
        for (int i = 0; i < 64; i++) {
            if (arp_cache_count < 64 || arp_cache[i].ip == 0) {
                arp_cache[arp_cache_count].ip = ip;
                memcpy(arp_cache[arp_cache_count].mac, arp->sender_mac, 6);
                arp_cache[arp_cache_count].expires = g_ticks + 60000;
                arp_cache_count++;
                break;
            }
        }
    }
}

void net_handle_ip(uint8_t *data, int len, uint8_t *src_mac) {
    (void)src_mac;
    if (len < (int)sizeof(ip_header_t)) return;
    ip_header_t *ip = (ip_header_t *)data;

    if ((ip->version_ihl & 0xF) < 5) return;
    if (ip->ttl == 0) return;

    uint32_t dst_ip = ntohl(*(uint32_t *)ip->dst);
    if (dst_ip != my_ip && dst_ip != 0xFFFFFFFF) return;

    uint8_t *payload = data + (ip->version_ihl & 0xF) * 4;
    int payload_len = ntohs(ip->total_len) - (ip->version_ihl & 0xF) * 4;

    switch (ip->protocol) {
        case IP_PROTO_ICMP:
            net_handle_icmp(payload, payload_len, ntohl(*(uint32_t *)ip->src));
            break;
        case IP_PROTO_TCP:
            net_handle_tcp(payload, payload_len, ntohl(*(uint32_t *)ip->src), ntohl(*(uint32_t *)ip->dst));
            break;
        case IP_PROTO_UDP:
            net_handle_udp(payload, payload_len, ntohl(*(uint32_t *)ip->src), ntohl(*(uint32_t *)ip->dst));
            break;
    }
}

void net_handle_icmp(uint8_t *data, int len, uint32_t src_ip) {
    if (len < (int)sizeof(icmp_header_t)) return;
    icmp_header_t *icmp = (icmp_header_t *)data;

    if (icmp->type == ICMP_ECHO_REQUEST) {
        icmp_header_t reply = *icmp;
        reply.type = ICMP_ECHO_REPLY;
        reply.code = 0;
        reply.checksum = 0;
        reply.checksum = ip_checksum(&reply, sizeof(icmp_header_t));

        ip_header_t ip;
        ip.version_ihl = 0x45;
        ip.tos = 0;
        ip.total_len = htons(sizeof(ip_header_t) + sizeof(icmp_header_t));
        ip.id = htons(ip_id++);
        ip.flags_frag = 0;
        ip.ttl = 64;
        ip.protocol = IP_PROTO_ICMP;
        ip.checksum = 0;
        *(uint32_t *)ip.src = htonl(my_ip);
        *(uint32_t *)ip.dst = htonl(src_ip);
        ip.checksum = ip_checksum(&ip, sizeof(ip_header_t));

        uint8_t pkt[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(icmp_header_t)];
        eth_header_t *eth = (eth_header_t *)pkt;
        memcpy(eth->dst, (uint8_t[6]){0x52,0x54,0x00,0x12,0x34,0x56}, 6);
        memcpy(eth->src, my_mac, 6);
eth->type = htons(ETH_P_IP);
        memcpy(pkt + sizeof(eth_header_t), &ip, sizeof(ip_header_t));
        memcpy(pkt + sizeof(eth_header_t) + sizeof(ip_header_t), &reply, sizeof(icmp_header_t));
        net_tx(pkt, sizeof(pkt));
    }
}

void net_handle_tcp(uint8_t *data, int len, uint32_t src_ip, uint32_t dst_ip) {
    (void)dst_ip;
    if (len < (int)sizeof(tcp_header_t)) return;
    tcp_header_t *tcp = (tcp_header_t *)data;

    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dst_port = ntohs(tcp->dst_port);

    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == TCP_STATE_ESTABLISHED &&
            sockets[i].local_port == dst_port &&
            sockets[i].remote_port == src_port &&
            sockets[i].remote_ip == src_ip) {
            // Found connection
            break;
        }
    }
}

void net_handle_udp(uint8_t *data, int len, uint32_t src_ip, uint32_t dst_ip) {
    (void)src_ip; (void)dst_ip;
    if (len < (int)sizeof(udp_header_t)) return;
    udp_header_t *udp = (udp_header_t *)data;

    uint16_t dst_port = ntohs(udp->dst_port);
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == 1 && sockets[i].type == SOCK_DGRAM &&
            sockets[i].local_port == dst_port) {
            break;
        }
    }
}

int arp_resolve(uint32_t ip, uint8_t *mac) {
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].ip == ip && g_ticks < arp_cache[i].expires) {
            memcpy(mac, arp_cache[i].mac, 6);
            return 0;
        }
    }

    arp_packet_t arp;
    arp.hw_type[0] = 0; arp.hw_type[1] = 1;
    arp.proto_type[0] = 0x08; arp.proto_type[1] = 0;
    arp.hw_size = 6;
    arp.proto_size = 4;
    arp.opcode = htons(ARP_OP_REQUEST);
    memcpy(arp.sender_mac, my_mac, 6);
    memcpy(arp.sender_ip, (uint8_t *)&my_ip, 4);
    memset(arp.target_mac, 0, 6);
    memcpy(arp.target_ip, (uint8_t *)&ip, 4);

    eth_header_t eth;
    memcpy(eth.dst, (uint8_t[6]){0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, 6);
    memcpy(eth.src, my_mac, 6);
    eth.type = htons(ETH_P_ARP);

    uint8_t pkt[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    memcpy(pkt, &eth, sizeof(eth_header_t));
    memcpy(pkt + sizeof(eth_header_t), &arp, sizeof(arp_packet_t));
    net_tx(pkt, sizeof(pkt));

    for (int i = 0; i < 1000000; i++) {
        for (int j = 0; j < arp_cache_count; j++) {
            if (arp_cache[j].ip == ip) {
                memcpy(mac, arp_cache[j].mac, 6);
                return 0;
            }
        }
        udelay(1000);
    }
    return -1;
}

int ip_send(uint32_t dst_ip, uint8_t protocol, void *data, int len) {
    uint8_t mac[6];
    if (arp_resolve(dst_ip, mac) != 0) return -1;

    ip_header_t ip;
    ip.version_ihl = 0x45;
    ip.tos = 0;
    ip.total_len = htons(sizeof(ip_header_t) + len);
    ip.id = htons(ip_id++);
    ip.flags_frag = 0;
    ip.ttl = 64;
    ip.protocol = protocol;
    ip.checksum = 0;
    *(uint32_t *)ip.src = htonl(my_ip);
    *(uint32_t *)ip.dst = htonl(dst_ip);
    ip.checksum = ip_checksum(&ip, sizeof(ip_header_t));

    eth_header_t eth;
    memcpy(eth.dst, mac, 6);
    memcpy(eth.src, my_mac, 6);
    eth.type = htons(ETH_P_IP);

    uint8_t pkt[sizeof(eth_header_t) + sizeof(ip_header_t) + len];
    memcpy(pkt, &eth, sizeof(eth_header_t));
    memcpy(pkt + sizeof(eth_header_t), &ip, sizeof(ip_header_t));
    memcpy(pkt + sizeof(eth_header_t) + sizeof(ip_header_t), data, len);

    net_tx(pkt, sizeof(pkt));
    return 0;
}

int icmp_send_echo(uint32_t dst_ip, uint16_t id, uint16_t seq, void *data, int len) {
    (void)data;
    icmp_header_t icmp;
    icmp.type = ICMP_ECHO_REQUEST;
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.rest = htonl((id << 16) | seq);
    icmp.checksum = ip_checksum(&icmp, sizeof(icmp_header_t) + len);
    return ip_send(dst_ip, IP_PROTO_ICMP, &icmp, sizeof(icmp_header_t) + len);
}

int udp_send(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, void *data, int len) {
    (void)data;
    udp_header_t udp;
    udp.src_port = htons(src_port);
    udp.dst_port = htons(dst_port);
    udp.length = htons(sizeof(udp_header_t) + len);
    udp.checksum = 0;
    return ip_send(dst_ip, IP_PROTO_UDP, &udp, sizeof(udp_header_t) + len);
}

int tcp_connect(uint32_t dst_ip, uint16_t dst_port) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == 0) {
            sockets[i].type = SOCK_STREAM;
            sockets[i].protocol = IP_PROTO_TCP;
            sockets[i].local_ip = my_ip;
            sockets[i].local_port = tcp_port_base++;
            sockets[i].remote_ip = dst_ip;
            sockets[i].remote_port = dst_port;
            sockets[i].state = TCP_STATE_SYN_SENT;

            tcp_header_t syn;
            syn.src_port = htons(sockets[i].local_port);
            syn.dst_port = htons(dst_port);
            syn.seq = htonl(tcp_seq++);
            syn.ack_seq = 0;
            syn.offset = 0x50;
            syn.flags = TCP_FLAG_SYN;
            syn.window = htons(65535);
            syn.checksum = 0;
            syn.urgent = 0;

            return ip_send(dst_ip, IP_PROTO_TCP, &syn, sizeof(tcp_header_t));
        }
    }
    return -1;
}

int socket(int domain, int type, int protocol) {
    (void)domain;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == 0) {
            sockets[i].type = type;
            sockets[i].protocol = protocol;
            sockets[i].local_ip = my_ip;
            sockets[i].local_port = 0;
            sockets[i].remote_port = 0;
            sockets[i].remote_ip = 0;
            sockets[i].state = 0;
            sockets[i].blocking = 1;
            return i;
        }
    }
    return -1;
}

int bind(int sockfd, const struct sockaddr *addr, int addrlen) {
    (void)addr; (void)addrlen;
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    return 0;
}

int listen(int sockfd, int backlog) {
    (void)backlog;
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    sockets[sockfd].state = TCP_STATE_LISTEN;
    return 0;
}

int accept(int sockfd, struct sockaddr *addr, int *addrlen) {
    (void)sockfd; (void)addr; (void)addrlen;
    return -1;
}

int connect(int sockfd, const struct sockaddr *addr, int addrlen) {
    (void)addr; (void)addrlen;
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    return tcp_connect(sockets[sockfd].remote_ip, sockets[sockfd].remote_port);
}

int send(int sockfd, const void *buf, int len, int flags) {
    (void)flags;
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    return tcp_send(sockfd, (void *)buf, len);
}

int recv(int sockfd, void *buf, int len, int flags) {
    (void)flags;
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    return tcp_recv(sockfd, buf, len);
}

int sendto(int sockfd, const void *buf, int len, int flags, const struct sockaddr *dest_addr, int addrlen) {
    (void)flags; (void)dest_addr; (void)addrlen;
    return send(sockfd, buf, len, 0);
}

int recvfrom(int sockfd, void *buf, int len, int flags, struct sockaddr *src_addr, int *addrlen) {
    (void)flags; (void)src_addr; (void)addrlen;
    return recv(sockfd, buf, len, 0);
}

int close(int sockfd) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    return tcp_close(sockfd);
}

int tcp_send(int sock, void *data, int len) {
    (void)sock; (void)data; (void)len;
    return len;
}

int tcp_recv(int sock, void *buf, int len) {
    (void)sock; (void)buf; (void)len;
    return 0;
}

int tcp_close(int sock) {
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    sockets[sock].state = 0;
    return 0;
}

uint16_t checksum(void *data, int len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len) sum += *(uint8_t *)ptr;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

uint16_t ip_checksum(void *data, int len) {
    return checksum(data, len);
}

uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, void *tcp_hdr, int tcp_len) {
    pseudo_header_t ph;
    ph.src_ip = htonl(src_ip);
    ph.dst_ip = htonl(dst_ip);
    ph.reserved = 0;
    ph.protocol = IP_PROTO_TCP;
    ph.length = htons(tcp_len);

    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)&ph;
    for (int i = 0; i < 6; i++) sum += ptr[i];

    uint16_t *tcp_ptr = (uint16_t *)tcp_hdr;
    for (int i = 0; i < tcp_len / 2; i++) sum += tcp_ptr[i];
    if (tcp_len & 1) sum += ((uint8_t *)tcp_hdr)[tcp_len - 1];

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}