#pragma once
#include "types.h"

struct sockaddr {
    uint16_t sa_family;
    char sa_data[14];
};

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8];
};

#define ETH_P_IP    0x0800
#define ETH_P_ARP   0x0806
#define ETH_P_RARP  0x8035
#define ETH_P_IPV6  0x86DD

#define IP_PROTO_ICMP  1
#define IP_PROTO_TCP   6
#define IP_PROTO_UDP   17

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

#define ICMP_ECHO_REQUEST  8
#define ICMP_ECHO_REPLY    0
#define ICMP_DEST_UNREACH  3
#define ICMP_TIME_EXCEEDED 11

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20

#define TCP_STATE_CLOSED       0
#define TCP_STATE_LISTEN       1
#define TCP_STATE_SYN_SENT     2
#define TCP_STATE_SYN_RECEIVED 3
#define TCP_STATE_ESTABLISHED  4
#define TCP_STATE_FIN_WAIT_1   5
#define TCP_STATE_FIN_WAIT_2   6
#define TCP_STATE_CLOSE_WAIT   7
#define TCP_STATE_CLOSING      8
#define TCP_STATE_LAST_ACK     9
#define TCP_STATE_TIME_WAIT    10

#define MAX_PACKET_SIZE 1514
#define MTU 1500

typedef struct {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__((packed)) eth_header_t;

typedef struct {
    uint8_t hw_type[2];
    uint8_t proto_type[2];
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint8_t sender_ip[4];
    uint8_t target_mac[6];
    uint8_t target_ip[4];
} __attribute__((packed)) arp_packet_t;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t src[4];
    uint8_t dst[4];
} __attribute__((packed)) ip_header_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t rest;
} __attribute__((packed)) icmp_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    uint8_t reserved;
    uint16_t length;
} __attribute__((packed)) pseudo_header_t;

typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t state;
    uint8_t flags;
    uint16_t window;
    uint32_t send_seq;
    uint32_t recv_seq;
    uint8_t *send_buf;
    uint32_t send_len;
    uint8_t *recv_buf;
    uint32_t recv_len;
} tcp_connection_t;

#define MAX_SOCKETS 64
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

typedef struct {
    int type;
    int protocol;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t local_ip;
    uint32_t remote_ip;
    int state;
    void *proto_data;
    int blocking;
} socket_t;

extern socket_t sockets[MAX_SOCKETS];
extern uint8_t my_mac[6];
extern uint32_t my_ip;

void net_init(void);
void net_rx(uint8_t *packet, int len);
void net_tx(uint8_t *packet, int len);

int eth_send(uint8_t *dst_mac, uint16_t type, void *data, int len);
int arp_resolve(uint32_t ip, uint8_t *mac);
int ip_send(uint32_t dst_ip, uint8_t protocol, void *data, int len);
int icmp_send_echo(uint32_t dst_ip, uint16_t id, uint16_t seq, void *data, int len);
int udp_send(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, void *data, int len);
void net_handle_udp(uint8_t *data, int len, uint32_t src_ip, uint32_t dst_ip);
void net_handle_tcp(uint8_t *data, int len, uint32_t src_ip, uint32_t dst_ip);
void net_handle_icmp(uint8_t *data, int len, uint32_t src_ip);
void net_handle_ip(uint8_t *data, int len, uint8_t *src_mac);
void net_handle_arp(uint8_t *data, int len, uint8_t *src_mac);
int tcp_connect(uint32_t dst_ip, uint16_t dst_port);
int tcp_send(int sock, void *data, int len);
int tcp_recv(int sock, void *buf, int len);
int tcp_close(int sock);

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, int addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, int *addrlen);
int connect(int sockfd, const struct sockaddr *addr, int addrlen);
int send(int sockfd, const void *buf, int len, int flags);
int recv(int sockfd, void *buf, int len, int flags);
int sendto(int sockfd, const void *buf, int len, int flags, const struct sockaddr *dest_addr, int addrlen);
int recvfrom(int sockfd, void *buf, int len, int flags, struct sockaddr *src_addr, int *addrlen);
int close(int sockfd);

uint16_t checksum(void *data, int len);
uint16_t ip_checksum(void *data, int len);
uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, void *tcp_hdr, int tcp_len);