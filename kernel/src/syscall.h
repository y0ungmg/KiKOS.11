#pragma once
#include "types.h"

#define SYSCALL_EXIT       1
#define SYSCALL_FORK       2
#define SYSCALL_EXECVE     3
#define SYSCALL_WAITPID    4
#define SYSCALL_GETPID     5
#define SYSCALL_GETPPID    6
#define SYSCALL_YIELD      7
#define SYSCALL_SLEEP      8
#define SYSCALL_KILL       9
#define SYSCALL_OPEN       10
#define SYSCALL_CLOSE      11
#define SYSCALL_READ       12
#define SYSCALL_WRITE      13
#define SYSCALL_LSEEK      14
#define SYSCALL_STAT       15
#define SYSCALL_MMAP       16
#define SYSCALL_MUNMAP     17
#define SYSCALL_IOCTL      18
#define SYSCALL_GETTIME    19
#define SYSCALL_SETTIME    20
#define SYSCALL_GETUID     20
#define SYSCALL_SETUID     21
#define SYSCALL_GETGID     22
#define SYSCALL_SETGID     23
#define SYSCALL_CHDIR      24
#define SYSCALL_GETCWD     25
#define SYSCALL_MKDIR      26
#define SYSCALL_RMDIR      27
#define SYSCALL_UNLINK     28
#define SYSCALL_RENAME     29
#define SYSCALL_PIPE       30
#define SYSCALL_DUP        31
#define SYSCALL_DUP2       32
#define SYSCALL_FCNTL      33
#define SYSCALL_FSTAT      34
#define SYSCALL_GETDENTS   35
#define SYSCALL_SELECT     36
#define SYSCALL_POLL       37
#define SYSCALL_SOCKET     38
#define SYSCALL_CONNECT    39
#define SYSCALL_BIND       40
#define SYSCALL_LISTEN     41
#define SYSCALL_ACCEPT     42
#define SYSCALL_SEND       43
#define SYSCALL_RECV       44
#define SYSCALL_SENDTO     45
#define SYSCALL_RECVFROM   46
#define SYSCALL_SHUTDOWN   47
#define SYSCALL_GETSOCKOPT 48
#define SYSCALL_SETSOCKOPT 49

typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
    uint32_t cr3;
    int syscall_num;
    int args[6];
} syscall_regs_t;

void syscall_init(void);
int syscall_handler(syscall_regs_t *regs);

#define SYSCALL_ARG0(regs) ((regs)->ebx)
#define SYSCALL_ARG1(regs) ((regs)->ecx)
#define SYSCALL_ARG2(regs) ((regs)->edx)
#define SYSCALL_ARG3(regs) ((regs)->esi)
#define SYSCALL_ARG4(regs) ((regs)->edi)
#define SYSCALL_ARG5(regs) ((regs)->ebp)

#define SYSCALL_RET(regs, val) ((regs)->eax = (val))