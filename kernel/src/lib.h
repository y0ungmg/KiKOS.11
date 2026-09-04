#pragma once

#include "types.h"

void *memset(void *dst, int v, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *memmove(void *dst, const void *src, u32 n);
int   memcmp(const void *a, const void *b, u32 n);
u32   strlen(const char *s);
int   strcmp(const char *a, const char *b);
int   strncmp(const char *a, const char *b, u32 n);
char *strcpy(char *d, const char *s);
char *strncpy(char *d, const char *s, u32 n);
char *strcat(char *d, const char *s);
char *strncat(char *d, const char *s, u32 n);

char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *hay, const char *needle);

int sscanf(const char *str, const char *fmt, ...);

void  utoa_dec(u32 v, char *buf);
void  itoa_dec(i32 v, char *buf);
void  utoa_hex(u32 v, char *buf);
u32   rand32(void);
void  rand_seed(u32 s);

void  io_wait(void);
void  outb(u16 port, u8 val);
u8    inb(u16 port);
void  outw(u16 port, u16 val);
u16   inw(u16 port);
u32   inl(u16 port);
void    outl(u16 port, u32 val);

uint16_t htons(uint16_t x);
uint16_t ntohs(uint16_t x);
uint32_t htonl(uint32_t x);
uint32_t ntohl(uint32_t x);
