BITS 32
global _start
global gdt_ptr
global idt_ptr
global page_directory
extern kmain
extern __bss_start
extern _end
extern dbg_init
extern gfx_init
extern idt_init
extern pic_remap
extern timer_init
extern rtc_init
extern kbd_init
extern gui_init
extern mouse_set_bounds
extern mouse_init
extern heap_init
extern vfs_init
extern pkg_init
extern av_init
extern fw_init

SECTION .bss
align 8
tss:
    resb 104
    resw 1
    resb 1024

align 4096
page_directory:
    resd 1024
page_tables:
    resd 1024 * 8
fb_page_table:
    resd 1024
kernel_stack:
    resb 16384
kernel_stack_top:

SECTION .text
_start:
    cli
    cld

    ; Bare serial init (for early crash debug before dbg_init)
    mov dx, 0x3FB
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8
    mov al, 0x01
    out dx, al
    mov dx, 0x3F9
    mov al, 0x00
    out dx, al
    mov dx, 0x3FB
    mov al, 0x03
    out dx, al
    mov dx, 0x3F9
    mov al, 0x00
    out dx, al
    mov dx, 0x3F8
    mov al, 0x42               ; 'B' = began
    out dx, al

    ; Clear BSS
    xor eax, eax
    mov edi, __bss_start
    mov ecx, _end
    sub ecx, edi
    shr ecx, 2
    rep stosd

    ; Stack
    mov esp, kernel_stack_top

    ; SSE
    mov eax, cr0
    and eax, 0xFFFEFFFF
    or eax, 0x20000
    mov cr0, eax
    mov eax, cr4
    or eax, 0x600
    mov cr4, eax

    ; Paging
    call setup_paging
    mov eax, page_directory
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Mask all IRQs before loading zero IDT
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

    ; GDT
    lgdt [gdt_ptr]

    ; TSS
    mov dword [tss + 4], kernel_stack_top
    mov word [tss + 8], 0x10
    mov word [tss + 104], 106
    mov eax, tss
    mov word [gdt_tss + 2], ax
    shr eax, 16
    mov byte [gdt_tss + 4], al
    mov byte [gdt_tss + 7], ah
    mov ax, 0x18
    ltr ax

    ; IDT (placeholder)
    lidt [idt_ptr]

    ; Segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, kernel_stack_top

    ; ---- Core init ----
    call dbg_init
    call heap_init
    call idt_init
    call pic_remap
    push dword 100             ; timer_init(100) - cdecl arg REQUIRED
    call timer_init
    add esp, 4
    call rtc_init
    call kbd_init

    ; ---- Kernel subsystems ----
    call vfs_init
    call pkg_init
    call av_init
    call fw_init

    ; ---- GUI (gfx_init MUST precede gui_init: sets fb pointer) ----
    push dword 0x7000          ; BootInfo*
    call gfx_init
    add esp, 4
    call gui_init
    push dword 768             ; mouse_set_bounds(1024, 768)
    push dword 1024
    call mouse_set_bounds
    add esp, 8
    call mouse_init

    ; Unmask IRQ0 (timer), IRQ1 (kbd), IRQ2 (cascade); IRQ12 (mouse) on slave
    mov al, 0xF8
    out 0x21, al
    mov al, 0xEF
    out 0xA1, al

    sti

    push dword 0x7000
    call kmain

.hang:
    cli
    hlt
    jmp .hang

setup_paging:
    mov edi, page_directory
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, page_directory
    mov eax, page_tables
    or eax, 0x3
    mov ecx, 8
.pd:
    stosd
    add eax, 4096
    loop .pd

    mov eax, fb_page_table
    or eax, 0x3
    mov [page_directory + 4048], eax

    mov edi, page_tables
    xor eax, eax
    or eax, 0x3
    mov ecx, 8 * 1024
.pt:
    stosd
    add eax, 4096
    loop .pt

    mov edi, fb_page_table
    mov eax, 0xFD000000
    or eax, 0x3
    mov ecx, 1024
.fb:
    stosd
    add eax, 4096
    loop .fb

    ret

gdt_start:
gdt_null:    dq 0
gdt_code:    dq 0x00CF9A000000FFFF
gdt_data:    dq 0x00CF92000000FFFF
gdt_tss:
    dw 1129
    dw 0
    db 0
    db 0x89
    db 0x00
    db 0
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

idt_start:
    times 256 dq 0
idt_end:

idt_ptr:
    dw idt_end - idt_start - 1
    dd idt_start
