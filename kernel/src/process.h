#pragma once
#include "types.h"
#include "paging.h"

#define MAX_PROCESSES 256
#define MAX_THREADS 1024
#define KERNEL_STACK_SIZE 8192
#define USER_STACK_SIZE 32768

#define PROC_STATE_RUNNING  0
#define PROC_STATE_READY    1
#define PROC_STATE_BLOCKED  2
#define PROC_STATE_ZOMBIE   3
#define PROC_STATE_SLEEPING 4

#define THREAD_STATE_RUNNING 0
#define THREAD_STATE_READY   1
#define THREAD_STATE_BLOCKED 2
#define THREAD_STATE_SLEEPING 3

#define PRIORITY_LOW      0
#define PRIORITY_NORMAL   5
#define PRIORITY_HIGH     10
#define PRIORITY_REALTIME 15

typedef uint32_t pid_t;
typedef uint32_t tid_t;

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
    uint32_t cr3;
} cpu_context_t;

typedef struct thread {
    tid_t tid;
    struct process *proc;
    cpu_context_t ctx;
    uint32_t state;
    uint32_t priority;
    uint32_t time_slice;
    uint32_t cpu_time;
    uint32_t *kernel_stack;
    void *user_stack;
    struct thread *next;
    struct thread *prev;
} thread_t;

typedef struct process {
    pid_t pid;
    char name[32];
    uint32_t state;
    thread_t *main_thread;
    thread_t *threads;
    int thread_count;
    uint32_t *page_directory;
    vaddr_t heap_start;
    vaddr_t heap_end;
    vaddr_t stack_start;
    int fd_table[64];
    struct process *parent;
    struct process *children;
    struct process *next_sibling;
    int exit_code;
    uint32_t cpu_time;
    uint32_t start_time;
} process_t;

extern process_t *current_process;
extern thread_t *current_thread;
extern process_t *process_list;
extern thread_t *ready_queue[16];
extern uint32_t next_pid;
extern uint32_t next_tid;

void scheduler_init(void);
void schedule(void);
void yield(void);

pid_t proc_create(const char *name, vaddr_t entry, void *arg);
void proc_exit(int code);
pid_t proc_get_pid(void);
process_t *proc_find(pid_t pid);
int proc_wait(pid_t pid, int *status);

tid_t thread_create(process_t *proc, vaddr_t entry, void *arg);
void thread_exit(void);
tid_t thread_get_tid(void);
void thread_sleep(uint32_t ms);
void thread_wake(thread_t *t);

void sched_add_ready(thread_t *t);
thread_t *sched_get_next(void);

int sys_fork(void);
int sys_execve(const char *path, char *const argv[], char *const envp[]);
int sys_exit(int code);
int sys_waitpid(pid_t pid, int *status, int options);
int sys_getpid(void);
int sys_getppid(void);
int sys_yield(void);
int sys_sleep(uint32_t ms);
int sys_kill(pid_t pid, int sig);