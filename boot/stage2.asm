%ifndef KERN_SECTORS
%define KERN_SECTORS 294
%endif
%ifndef KERN_BYTES
%define KERN_BYTES (KERN_SECTORS*512)
%endif

%define VBE_MODE_WIDTH  1024
%define VBE_MODE_HEIGHT 768
%define VBE_MODE_BPP    32

BITS 16
ORG 0x7E00

; ========== Entry point (must be first!) ==========
start:
    jmp real_start

; ========== Data section ==========
msg_s2       db "stage2: memory, vbe, loading kernel", 13, 10, 0
msg_pm       db "entering protected mode", 13, 10, 0
msg_derr     db "kernel load error", 13, 10, 0
msg_verr     db "vbe setup failed", 13, 10, 0
msg_mem      db "detecting memory (e820)...", 13, 10, 0
msg_mem_ok   db "memory map done", 13, 10, 0
msg_mem_map  db "Memory map:", 13, 10, 0

boot_drive db 0
mlist_off  dw 0
mlist_seg  dw 0
kremain    dw 0
retries    db 0

align 8
dapk:
    db 0x10, 0
    dw 64
    dw 0x0000
    dw 0x2000
    dq 64

; 16-bit GDT
gdt16:
    dq 0
    dq 0x00009A000000FFFF
    dq 0x000092000000FFFF
gdt16_desc:
    dw 23
    dd gdt16

; 32-bit GDT
gdt32:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt32_desc:
    dw 23
    dd gdt32

; VBE structures
vbe_info:
    times 512 db 0
mode_info:
    times 256 db 0
edid_buffer:
    times 128 db 0

fb_addr    dd 0
fb_pitch   dw 0
fb_width   dw 0
fb_height  dw 0
fb_bpp     db 0
fb_type    db 0
has_edid   db 0
edid_h_active dw 0
edid_v_active dw 0

mem_map:
    times 512 db 0
mem_entries dd 0
mem_count   dw 0

KERN_LBA    equ 64
KERN_SEG    equ 0x2000
KERN_OFF    equ 0x0000

; ========== Code section ==========
real_start:
    mov [boot_drive], dl
    mov si, msg_s2
    call print

    call detect_memory_e820
    call enable_a20_fast
    call setup_gdt_16
    call vbe_setup
    jc fatal_vbe

    call load_kernel_chunks
    jc fatal_disk

    mov si, msg_pm
    call print

    ; Switch to protected mode
    cli
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:pm_entry

fatal_vbe:
    mov si, msg_verr
    call print
fatal_disk:
    mov si, msg_derr
    call print
fatal:
    cli
.hlt: hlt
    jmp .hlt

; ========== 16-bit utilities ==========
print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print
.done: ret

print_hex16:
    pusha
    mov cx, 4
.loop:
    rol dx, 4
    mov al, dl
    and al, 0xF
    cmp al, 10
    jl .digit
    add al, 'A' - 10
    jmp .pr
.digit: add al, '0'
.pr:  mov ah, 0x0E; int 0x10; dec cx; jnz .loop; popa; ret

print_hex32:
    pusha
    mov cx, 8
.loop32:
    rol eax, 4
    mov dl, al
    and dl, 0xF
    cmp dl, 10
    jl .d32
    add dl, 'A' - 10
    jmp .p32
.d32: add dl, '0'
.p32: mov ah, 0x0E; mov bh, 0; mov al, dl; int 0x10; dec cx; jnz .loop32; popa; ret

print_mem_map:
    pusha
    mov si, msg_mem_map
    call print
    mov si, mem_map
    mov cx, [mem_count]
.walk:
    mov eax, [si]
    mov edx, [si+4]
    mov ecx, [si+8]
    mov ebx, [si+12]
    push cx
    mov dx, ax
    call print_hex32
    mov al, '-'; mov ah, 0x0E; int 0x10
    mov dx, bx
    call print_hex32
    mov al, ' '; mov ah, 0x0E; int 0x10
    pop cx
    add si, 20
    loop .walk
    popa
    ret

; ========== Memory detection (E820) ==========
detect_memory_e820:
    mov si, msg_mem
    call print
    xor ebx, ebx
    mov di, mem_map
    mov dword [mem_entries], 0
.loop:
    mov eax, 0xE820
    mov ecx, 20
    mov edx, 0x534D4150
    int 0x15
    jc .done
    add di, 20
    inc dword [mem_entries]
    cmp ebx, 0
    jne .loop
.done:
    mov si, msg_mem_ok
    call print
    ret

enable_a20_fast:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

setup_gdt_16:
    lgdt [gdt16_desc]
    ret

; ========== VBE 2.0+ with EDID ==========
vbe_setup:
    push es
    push ds
    pop es                     ; ES = DS = 0

    ; Set VBE2 signature
    mov di, vbe_info
    mov word [di], 0x4256     ; 'VB'
    mov word [di+2], 0x4532   ; 'E2'

    ; Get VBE info
    mov di, vbe_info
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .fail

    ; Get mode list pointer
    mov ax, [vbe_info + 14]
    mov [mlist_off], ax
    mov ax, [vbe_info + 16]
    mov [mlist_seg], ax

    ; Walk mode list looking for 1024x768x32
    mov si, [mlist_off]
.walk:
    mov gs, [mlist_seg]
    mov cx, [gs:si]
    add si, 2
    cmp cx, 0xFFFF
    je .fail
    cmp cx, 0x0000
    je .walk
    push si
    mov di, mode_info
    mov ax, 0x4F01
    int 0x10
    pop si
    cmp ax, 0x004F
    jne .walk

    cmp word [mode_info + 18], VBE_MODE_WIDTH
    jne .walk
    cmp word [mode_info + 20], VBE_MODE_HEIGHT
    jne .walk
    cmp byte [mode_info + 25], VBE_MODE_BPP
    jne .walk
    test byte [mode_info], 0x81
    jz .walk

    ; Found! Set the mode
.found:
    mov bx, cx
    or bx, 0x4000            ; LFB bit
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .fail

    ; Save framebuffer info
    mov eax, [mode_info + 40]
    mov [fb_addr], eax
    mov ax, [mode_info + 16]
    mov [fb_pitch], ax
    mov ax, [mode_info + 18]
    mov [fb_width], ax
    mov ax, [mode_info + 20]
    mov [fb_height], ax
    mov al, [mode_info + 25]
    mov [fb_bpp], al
    mov byte [fb_type], 1

    pop es
    clc
    ret
.fail:
    pop es
    stc
    ret

probe_edid:
    mov ax, 0x4F15
    mov bl, 0
    mov cx, 0
    mov di, edid_buffer
    int 0x10
    cmp ax, 0x004F
    jne .no_edid
    mov byte [has_edid], 1
    mov si, edid_buffer + 0x36
    lodsw
    mov [edid_h_active], ax
    lodsw
    mov [edid_v_active], ax
.no_edid:
    ret

; ========== Load kernel in chunks ==========
load_kernel_chunks:
    mov word [kremain], KERN_SECTORS
    mov byte [retries], 3
    mov dword [dapk + 8], KERN_LBA

.chunk:
    mov ax, [kremain]
    test ax, ax
    jz .done
    cmp ax, 64
    jbe .setcnt
    mov ax, 64
.setcnt:
    mov [dapk + 2], ax

    mov si, dapk
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .advance

    dec byte [retries]
    jz .err
    xor ah, ah
    int 0x13
    jmp .chunk

.advance:
    mov ax, [dapk + 2]
    sub [kremain], ax
    add [dapk + 8], ax
    adc word [dapk + 10], 0
    add word [dapk + 6], 0x800
    jmp .chunk

.done:
    clc
    ret
.err:
    stc
    ret

; ========== Messages (defined at top, not here) ==========
KERN_SEG    equ 0x2000
KERN_OFF    equ 0x0000

; ========== Protected mode entry ==========
BITS 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9F000

; Copy kernel from 0x20000 to 0x100000
    mov esi, KERN_SEG << 4
    mov edi, 0x100000
    mov ecx, (KERN_SECTORS * 512) / 4
    cld
    rep movsd

    ; Fill boot_info struct at 0x7000 (agreed upon by bootloader and kernel)
    ; Layout: fb_addr, fb_pitch, fb_width, fb_height, fb_bpp, ext_mem_kb, mem_count, mem_map, boot_drive
    mov edi, 0x7000
    mov eax, [fb_addr]
    stosd
    movzx eax, word [fb_pitch]
    stosd
    movzx eax, word [fb_width]
    stosd
    movzx eax, word [fb_height]
    stosd
    movzx eax, byte [fb_bpp]
    stosd
    mov eax, [mem_entries]
    stosd
    mov eax, mem_map
    stosd
    movzx eax, byte [boot_drive]
    stosd
    xor eax, eax
    stosd

    ; Call kernel with boot_info pointer
    mov eax, 0x7000
    push eax

    mov eax, 0x100000
    jmp eax

.hang:
    hlt
    jmp .hang