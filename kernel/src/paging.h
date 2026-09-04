#pragma once
#include "types.h"

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PAGE_PRESENT    0x001
#define PAGE_WRITE      0x002
#define PAGE_USER       0x004
#define PAGE_PWT        0x008
#define PAGE_PCD        0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_PAT        0x080
#define PAGE_GLOBAL     0x100
#define PAGE_SIZE_4M    0x080

#define KERNEL_VIRT_BASE 0xC0000000
#define KERNEL_PHYS_BASE 0x00100000
#define USER_VIRT_BASE   0x00000000
#define USER_STACK_TOP   0xBFFFFFFF

typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;

typedef union {
    uint32_t raw;
    struct {
        uint32_t present    : 1;
        uint32_t write      : 1;
        uint32_t user       : 1;
        uint32_t pwt        : 1;
        uint32_t pcd        : 1;
        uint32_t accessed   : 1;
        uint32_t dirty      : 1;
        uint32_t pat        : 1;
        uint32_t global     : 1;
        uint32_t avail      : 3;
        uint32_t frame      : 20;
    };
} page_table_entry_t;

typedef union {
    uint32_t raw;
    struct {
        uint32_t present    : 1;
        uint32_t write      : 1;
        uint32_t user       : 1;
        uint32_t pwt        : 1;
        uint32_t pcd        : 1;
        uint32_t accessed   : 1;
        uint32_t reserved   : 1;
        uint32_t size       : 1;
        uint32_t ignored    : 1;
        uint32_t avail      : 3;
        uint32_t frame      : 10;
        uint32_t reserved2  : 9;
    };
} page_dir_entry_t;

extern uint32_t *page_directory;
extern uint32_t *page_tables;

void paging_init(void);
void paging_enable(void);
void paging_map_page(vaddr_t virt, paddr_t phys, uint32_t flags);
void paging_unmap_page(vaddr_t virt);
paddr_t paging_get_physical(vaddr_t virt);
void paging_invalidate(vaddr_t virt);

void *kmalloc_page(void);
void kfree_page(void *ptr);
void *kmalloc_pages(int count);
void kfree_pages(void *ptr, int count);

uint32_t get_free_page_count(void);
uint32_t get_used_page_count(void);

typedef struct {
    uint32_t total_pages;
    uint32_t free_pages;
    uint32_t used_pages;
    uint32_t kernel_pages;
    uint32_t user_pages;
} memory_stats_t;

void get_memory_stats(memory_stats_t *stats);