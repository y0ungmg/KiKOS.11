#include "process.h"
#include "paging.h"
#include "lib.h"
#include "timer.h"
#include "mm.h"
#include "vfs.h"
#include "com1.h"

process_t *current_process = 0;
thread_t *current_thread = 0;
process_t *process_list = 0;
thread_t *ready_queue[16] = {0};
uint32_t next_pid = 1;
uint32_t next_tid = 1;

static process_t *alloc_process(void) {
    process_t *p = (process_t *)kmalloc_page();
    if (!p) return 0;
    memset(p, 0, sizeof(process_t));
    return p;
}

static thread_t *alloc_thread(void) {
    thread_t *t = (thread_t *)kmalloc_page();
    if (!t) return 0;
    memset(t, 0, sizeof(thread_t));
    t->kernel_stack = (uint32_t *)((uint32_t)kmalloc_page() + 4096);
    return t;
}

void free_process(process_t *p) {
    if (p->page_directory) {
        for (int i = 0; i < 1024; i++) {
            if (p->page_directory[i] & 1) {
                uint32_t pt = p->page_directory[i] & 0xFFFFF000;
                for (int j = 0; j < 1024; j++) {
                    uint32_t *pt_virt = (uint32_t *)(pt + KERNEL_VIRT_BASE);
                    if (pt_virt[j] & 1) {
                        uint32_t frame = pt_virt[j] & 0xFFFFF000;
                        if (frame >= KERNEL_PHYS_BASE) {
                            kfree_page((void *)(frame + KERNEL_VIRT_BASE));
                        }
                    }
                }
                kfree_page((void *)(pt + KERNEL_VIRT_BASE));
            }
        }
        kfree_page((void *)p->page_directory);
    }
    kfree_page(p);
}

void scheduler_init(void) {
    process_t *init = alloc_process();
    if (!init) return;
    init->pid = next_pid++;
    strcpy(init->name, "init");
    init->state = PROC_STATE_RUNNING;
    init->page_directory = (uint32_t *)0x9C000;
    init->start_time = g_ticks;

    thread_t *main = alloc_thread();
    if (!main) return;
    main->tid = next_tid++;
    main->proc = init;
    main->state = THREAD_STATE_RUNNING;
    main->priority = PRIORITY_NORMAL;
    main->time_slice = 10;
    main->user_stack = kmalloc_page();
    init->main_thread = main;
    init->threads = main;

    current_process = init;
    current_thread = main;
    process_list = init;
    ready_queue[PRIORITY_NORMAL] = main;
    main->next = main;
    main->prev = main;
}

void schedule(void) {
    if (!current_thread) return;

    thread_t *next = 0;
    for (int pri = 15; pri >= 0; pri--) {
        if (ready_queue[pri]) {
            next = ready_queue[pri];
            ready_queue[pri] = next->next;
            if (next->next == next) {
                ready_queue[pri] = 0;
            } else {
                next->prev->next = next->next;
                next->next->prev = next->prev;
            }
            break;
        }
    }

    if (!next) next = current_thread;

    if (next != current_thread) {
        thread_t *old = current_thread;
        current_thread = next;
        current_process = next->proc;

        asm volatile(
            "mov %%esp, %0\n"
            "mov %%ebp, %1\n"
            "mov %%eax, %2\n"
            "mov %%ebx, %3\n"
            "mov %%ecx, %4\n"
            "mov %%edx, %5\n"
            "mov %%esi, %6\n"
            "mov %%edi, %7\n"
            "movl $1f, %8\n"
            "pushfl\n"
            "pop %9\n"
            "1:\n"
            : "=m"(old->ctx.esp), "=m"(old->ctx.ebp), "=m"(old->ctx.eax),
              "=m"(old->ctx.ebx), "=m"(old->ctx.ecx), "=m"(old->ctx.edx),
              "=m"(old->ctx.esi), "=m"(old->ctx.edi), "=m"(old->ctx.eip),
              "=m"(old->ctx.eflags)
        );

asm volatile(
            "mov %0, %%eax\n"
            "mov %%eax, %%cr3\n"
            "mov %1, %%esp\n"
            "mov %2, %%ebp\n"
            "mov %3, %%eax\n"
            "mov %4, %%ebx\n"
            "mov %5, %%ecx\n"
            "mov %6, %%edx\n"
            "mov %7, %%esi\n"
            "mov %8, %%edi\n"
            "jmp *%9\n"
            :
            : "m"(next->ctx.cr3), "m"(next->ctx.esp), "m"(next->ctx.ebp),
              "m"(next->ctx.eax), "m"(next->ctx.ebx), "m"(next->ctx.ecx),
              "m"(next->ctx.edx), "m"(next->ctx.esi), "m"(next->ctx.edi),
              "m"(next->ctx.eip)
        );
    }
}

void yield(void) {
    schedule();
}

pid_t proc_create(const char *name, vaddr_t entry, void *arg) {
    (void)arg;
    process_t *p = alloc_process();
    if (!p) return -1;

    p->pid = next_pid++;
    strncpy(p->name, name, 31);
    p->state = PROC_STATE_READY;
    p->start_time = g_ticks;

    p->page_directory = (uint32_t *)kmalloc_page();
    if (!p->page_directory) {
        kfree_page(p);
        return -1;
    }
    memcpy(p->page_directory, (void *)0x9C000, 4096);

    thread_t *t = alloc_thread();
    if (!t) {
        kfree_page(p->page_directory);
        kfree_page(p);
        return -1;
    }

    t->tid = next_tid++;
    t->proc = p;
    t->state = THREAD_STATE_READY;
    t->priority = PRIORITY_NORMAL;
    t->time_slice = 10;
    t->ctx.eip = entry;
    t->ctx.esp = (uint32_t)t->user_stack + 4096;
    t->ctx.eflags = 0x202;
    t->ctx.cs = 0x1B;
    t->ctx.ds = t->ctx.es = t->ctx.fs = t->ctx.gs = t->ctx.ss = 0x23;
    t->ctx.cr3 = (uint32_t)p->page_directory - KERNEL_VIRT_BASE;
    t->user_stack = kmalloc_page();

    p->main_thread = t;
    p->threads = t;

    p->next_sibling = process_list;
    process_list = p;

    sched_add_ready(t);

    return p->pid;
}

void proc_exit(int code) {
    if (!current_process) return;

    current_process->state = PROC_STATE_ZOMBIE;
    current_process->exit_code = code;
    current_thread->state = THREAD_STATE_BLOCKED;

    process_t *p = process_list;
    process_t *prev = 0;
    while (p) {
        if (p == current_process) {
            if (prev) prev->next_sibling = p->next_sibling;
            else process_list = p->next_sibling;
            break;
        }
        prev = p;
        p = p->next_sibling;
    }

    schedule();
}

pid_t proc_get_pid(void) {
    return current_process ? current_process->pid : 0;
}

process_t *proc_find(pid_t pid) {
    for (process_t *p = process_list; p; p = p->next_sibling) {
        if (p->pid == pid) return p;
    }
    return 0;
}

int proc_wait(pid_t pid, int *status) {
    while (1) {
        process_t *child = proc_find(pid);
        if (!child) return -1;
        if (child->state == PROC_STATE_ZOMBIE) {
            if (status) *status = child->exit_code;
            free_process(child);
            return pid;
        }
        yield();
    }
    return -1;
}

thread_t *alloc_thread_internal(process_t *proc, vaddr_t entry, void *arg) {
    (void)arg;
    thread_t *t = alloc_thread();
    if (!t) return 0;

    t->tid = next_tid++;
    t->proc = proc;
    t->state = THREAD_STATE_READY;
    t->priority = PRIORITY_NORMAL;
    t->time_slice = 10;
    t->ctx.eip = entry;
    t->ctx.esp = (uint32_t)t->user_stack + 4096;
    t->ctx.eflags = 0x202;
    t->ctx.cs = 0x1B;
    t->ctx.ds = t->ctx.es = t->ctx.fs = t->ctx.gs = t->ctx.ss = 0x23;
    t->ctx.cr3 = (uint32_t)proc->page_directory - KERNEL_VIRT_BASE;
    t->user_stack = kmalloc_page();

    t->next = proc->threads;
    proc->threads = t;
    proc->thread_count++;

    return t;
}

tid_t thread_create(process_t *proc, vaddr_t entry, void *arg) {
    thread_t *t = alloc_thread_internal(proc, entry, arg);
    if (!t) return -1;
    sched_add_ready(t);
    return t->tid;
}

void thread_exit(void) {
    if (!current_thread) return;
    current_thread->state = THREAD_STATE_BLOCKED;
    schedule();
}

tid_t thread_get_tid(void) {
    return current_thread ? current_thread->tid : 0;
}

void thread_sleep(uint32_t ms) {
    if (!current_thread) return;
    uint32_t wake = g_ticks + ms;
    current_thread->state = THREAD_STATE_SLEEPING;
    current_thread->cpu_time = wake;
    schedule();
}

void thread_wake(thread_t *t) {
    if (t && t->state == THREAD_STATE_SLEEPING) {
        if (g_ticks >= t->cpu_time) {
            t->state = THREAD_STATE_READY;
            sched_add_ready(t);
        }
    }
}

void sched_add_ready(thread_t *t) {
    int pri = t->priority;
    if (pri > 15) pri = 15;
    if (pri < 0) pri = 0;

    if (!ready_queue[pri]) {
        ready_queue[pri] = t;
        t->next = t;
        t->prev = t;
    } else {
        t->next = ready_queue[pri];
        t->prev = ready_queue[pri]->prev;
        ready_queue[pri]->prev->next = t;
        ready_queue[pri]->prev = t;
    }
}

thread_t *sched_get_next(void) {
    for (int pri = 15; pri >= 0; pri--) {
        if (ready_queue[pri]) {
            thread_t *t = ready_queue[pri];
            ready_queue[pri] = t->next;
            if (t->next == t) ready_queue[pri] = 0;
            else {
                t->prev->next = t->next;
                t->next->prev = t->prev;
            }
            return t;
        }
    }
    return current_thread;
}

int sys_fork(void) {
    return proc_create("fork", (vaddr_t)current_thread->ctx.eip, 0);
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    (void)path; (void)argv; (void)envp;
    return -1;
}

int sys_exit(int code) {
    proc_exit(code);
    return 0;
}

int sys_waitpid(pid_t pid, int *status, int options) {
    (void)options;
    return proc_wait(pid, status);
}

int sys_getpid(void) {
    return proc_get_pid();
}

int sys_getppid(void) {
    return current_process ? current_process->parent ? current_process->parent->pid : 0 : 0;
}

int sys_yield(void) {
    yield();
    return 0;
}

int sys_sleep(uint32_t ms) {
    thread_sleep(ms);
    return 0;
}

int sys_kill(pid_t pid, int sig) {
    (void)sig;
    process_t *p = proc_find(pid);
    if (!p) return -1;
    proc_exit(128 + sig);
    return 0;
}