#include "lib.h"
#include "mm.h"

void *memset(void *dst, int v, u32 n)
{
    u8 *d = (u8 *)dst;
    while (n--) *d++ = (u8)v;
    return dst;
}

void *memcpy(void *dst, const void *src, u32 n)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, u32 n)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, u32 n)
{
    const u8 *x = (const u8 *)a;
    const u8 *y = (const u8 *)b;
    while (n--) {
        if (*x != *y) return *x - *y;
        x++; y++;
    }
    return 0;
}

u32 strlen(const char *s)
{
    u32 n = 0;
    while (*s++) n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (u8)*a - (u8)*b;
}

int strncmp(const char *a, const char *b, u32 n)
{
    while (n-- && *a) {
        if (*a != *b) return (u8)*a - (u8)*b;
        a++; b++;
    }
    return 0;
}

char *strcpy(char *d, const char *s)
{
    char *p = d;
    while ((*p++ = *s++)) ;
    return d;
}

char *strncpy(char *d, const char *s, u32 n)
{
    u32 i = 0;
    for (; i < n && s[i]; i++) d[i] = s[i];
    for (; i < n; i++) d[i] = 0;
    return d;
}

char *strcat(char *d, const char *s)
{
    char *p = d;
    while (*p) p++;
    while ((*p++ = *s++)) ;
    return d;
}

char *strncat(char *d, const char *s, u32 n)
{
    char *p = d;
    while (*p) p++;
    while (n-- && (*p = *s++)) p++;
    *p = 0;
    return d;
}

void utoa_dec(u32 v, char *buf)
{
    char tmp[12];
    int i = 0, j = 0;
    if (!v) tmp[i++] = '0';
    while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
    while (i) buf[j++] = tmp[--i];
    buf[j] = 0;
}

void itoa_dec(i32 v, char *buf)
{
    if (v < 0) {
        *buf++ = '-';
        utoa_dec((u32)(-v), buf);
    } else {
        utoa_dec((u32)v, buf);
    }
}

void utoa_hex(u32 v, char *buf)
{
    static const char h[] = "0123456789ABCDEF";
    char tmp[9];
    int i = 0, j = 0;
    if (!v) tmp[i++] = '0';
    while (v) { tmp[i++] = h[v & 0xF]; v >>= 4; }
    while (i) buf[j++] = tmp[--i];
    buf[j] = 0;
}

u32 rng_state = 0x1234ABCD;

void rand_seed(u32 s)
{
    rng_state = s ? s : 1;
}

u32 rand32(void)
{
    u32 x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

void io_wait(void)
{
    outb(0x80, 0);
}

void outb(u16 port, u8 val)
{
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

u8 inb(u16 port)
{
    u8 r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

void outw(u16 port, u16 val)
{
    __asm__ volatile("outw %0, %1" :: "a"(val), "Nd"(port));
}

u16 inw(u16 port)
{
    u16 r;
    __asm__ volatile("inw %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    if (c == 0) return (char *)s;
    return 0;
}

char *strrchr(const char *s, int c)
{
    const char *last = 0;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == 0) return (char *)s;
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++; n++;
        }
        if (!*n) return (char *)haystack;
    }
    return 0;
}

int sscanf(const char *str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int count = 0;
    while (*fmt && *str) {
        if (*fmt == '%' && *(fmt + 1) == 's') {
            char **arg = va_arg(args, char **);
            while (*str == ' ') str++;
            char *start = (char *)str;
            while (*str && *str != ' ') str++;
            int len = str - start;
            *arg = (char *)kmalloc(len + 1);
            if (!*arg) { va_end(args); return count; }
            memcpy(*arg, start, len);
            (*arg)[len] = 0;
            count++;
            fmt += 2;
        } else {
            fmt++;
        }
    }
    va_end(args);
    return count;
}

int snprintf(char *str, int size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = 0;
    while (*fmt && written < size - 1) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            if (*fmt == 's') {
                char *s = va_arg(args, char *);
                while (*s && written < size - 1) {
                    str[written++] = *s++;
                }
            } else if (*fmt == 'd' || *fmt == 'i') {
                int d = va_arg(args, int);
                char buf[16];
                itoa_dec(d, buf);
                for (int i = 0; buf[i] && written < size - 1; i++)
                    str[written++] = buf[i];
            } else if (*fmt == 'u') {
                u32 d = va_arg(args, u32);
                char buf[16];
                utoa_dec(d, buf);
                for (int i = 0; buf[i] && written < size - 1; i++)
                    str[written++] = buf[i];
            } else if (*fmt == 'x') {
                u32 d = va_arg(args, u32);
                char buf[16];
                utoa_hex(d, buf);
                for (int i = 0; buf[i] && written < size - 1; i++)
                    str[written++] = buf[i];
            } else if (*fmt == 'c') {
                int c = va_arg(args, int);
                if (written < size - 1) str[written++] = (char)c;
            } else if (*fmt == '%') {
                if (written < size - 1) str[written++] = '%';
            }
        } else {
            str[written++] = *fmt;
        }
        fmt++;
    }
    str[written] = 0;
    va_end(args);
    return written;
}

uint16_t htons(uint16_t x) {
    return (x >> 8) | (x << 8);
}

uint16_t ntohs(uint16_t x) {
    return htons(x);
}

uint32_t htonl(uint32_t x) {
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24);
}

uint32_t ntohl(uint32_t x) {
    return htonl(x);
}

u32 inl(u16 port) {
    u32 r;
    __asm__ volatile("inl %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

void outl(u16 port, u32 val) {
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"(port));
}
