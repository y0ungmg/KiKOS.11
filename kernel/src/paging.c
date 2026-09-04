#include "paging.h"
#include "lib.h"
#include "mm.h"

extern uint32_t *page_directory;
extern uint32_t *page_tables;

static uint32_t *page_frame_bitmap = 0;
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;
static uint32_t kernel_end_frame = 0;

static inline void set_frame(uint32_t frame) {
    page_frame_bitmap[frame / 32] |= (1 << (frame % 32));
}

static inline void clear_frame(uint32_t frame) {
    page_frame_bitmap[frame / 32] &= ~(1 << (frame % 32));
}

static inline int test_frame(uint32_t frame) {
    return page_frame_bitmap[frame / 32] & (1 << (frame % 32));
}

static uint32_t first_free_frame(void) {
    for (uint32_t i = 0; i < total_frames / 32; i++) {
        if (page_frame_bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(page_frame_bitmap[i] & (1 << j))) {
                    return i * 32 + j;
                }
            }
        }
    }
    return 0xFFFFFFFF;
}

void paging_init(void) {
    extern uint32_t _end;
    uint32_t kernel_end = (uint32_t)&_end;
    kernel_end_frame = (kernel_end + 4095) / 4096;

    total_frames = 1024 * 1024;
    page_frame_bitmap = (uint32_t *)0x9E000;
    memset(page_frame_bitmap, 0, total_frames / 8);

    for (uint32_t i = 0; i < kernel_end_frame; i++) {
        set_frame(i);
        used_frames++;
    }

    set_frame(0);
    used_frames++;
}

void paging_enable(void) {
    asm volatile("mov %0, %%cr3" :: "r"(0x9C000));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

void paging_map_page(vaddr_t virt, paddr_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    uint32_t *pd = (uint32_t *)0x9C000;
    uint32_t *pt = (uint32_t *)(pd[pd_index] & 0xFFFFF000);

    if (!(pd[pd_index] & 1)) {
        uint32_t frame = first_free_frame();
        if (frame == 0xFFFFFFFF) return;
        set_frame(frame);
        pd[pd_index] = (frame * 4096) | 3;
        memset((void *)((frame * 4096) + KERNEL_VIRT_BASE), 0, 4096);
    }

    uint32_t *pt_virt = (uint32_t *)((pd[pd_index] & 0xFFFFF000) + KERNEL_VIRT_BASE);
    pt_virt[pt_index] = (phys & 0xFFFFF000) | flags | 1;

    asm volatile("invlpg %0" :: "m"(*(char *)virt));
}

void paging_unmap_page(vaddr_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    uint32_t *pd = (uint32_t *)0x9C000;
    if (!(pd[pd_index] & 1)) return;

    uint32_t *pt_virt = (uint32_t *)((pd[pd_index] & 0xFFFFF000) + KERNEL_VIRT_BASE);
    uint32_t frame = pt_virt[pt_index] & 0xFFFFF000;
    pt_virt[pt_index] = 0;

    if (frame >= KERNEL_PHYS_BASE) {
        clear_frame(frame / 4096);
        used_frames--;
    }

    asm volatile("invlpg %0" :: "m"(*(char *)virt));
}

paddr_t paging_get_physical(vaddr_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    uint32_t *pd = (uint32_t *)0x9C000;
    if (!(pd[pd_index] & 1)) return 0;

    uint32_t *pt_virt = (uint32_t *)((pd[pd_index] & 0xFFFFF000) + KERNEL_VIRT_BASE);
    return (pt_virt[pt_index] & 0xFFFFF000) | (virt & 0xFFF);
}

void paging_invalidate(vaddr_t virt) {
    asm volatile("invlpg %0" :: "m"(*(char *)virt));
}

void *kmalloc_page(void) {
    uint32_t frame = first_free_frame();
    if (frame == 0xFFFFFFFF) return 0;
    set_frame(frame);
    used_frames++;
    void *ptr = (void *)(frame * 4096 + KERNEL_VIRT_BASE);
    memset(ptr, 0, 4096);
    return ptr;
}

void kfree_page(void *ptr) {
    paddr_t phys = (paddr_t)ptr - KERNEL_VIRT_BASE;
    uint32_t frame = phys / 4096;
    if (frame < kernel_end_frame) return;
    clear_frame(frame);
    used_frames--;
}

void *kmalloc_pages(int count) {
    if (count <= 0) return 0;
    if (count == 1) return kmalloc_page();

    int consecutive = 0;
    uint32_t start_frame = 0xFFFFFFFF;

    for (uint32_t i = 0; i < total_frames; i++) {
        if (!test_frame(i)) {
            if (consecutive == 0) start_frame = i;
            consecutive++;
            if (consecutive == count) break;
        } else {
            consecutive = 0;
            start_frame = 0xFFFFFFFF;
        }
    }

    if (start_frame == 0xFFFFFFFF) return 0;

    for (int i = 0; i < count; i++) {
        set_frame(start_frame + i);
    }
    used_frames += count;
    return (void *)(start_frame * 4096 + KERNEL_VIRT_BASE);
}

void kfree_pages(void *ptr, int count) {
    if (!ptr || count <= 0) return;
    paddr_t phys = (paddr_t)ptr - KERNEL_VIRT_BASE;
    uint32_t frame = phys / 4096;
    for (int i = 0; i < count; i++) {
        if (frame + i >= kernel_end_frame) {
            clear_frame(frame + i);
            used_frames--;
        }
    }
}

uint32_t get_free_page_count(void) {
    return total_frames - used_frames;
}

uint32_t get_used_page_count(void) {
    return used_frames;
}

void get_memory_stats(memory_stats_t *stats) {
    stats->total_pages = total_frames;
    stats->free_pages = total_frames - used_frames;
    stats->used_pages = used_frames;
    stats->kernel_pages = kernel_end_frame;
    stats->user_pages = used_frames - kernel_end_frame;
}