%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 10
%endif

BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    cld

    mov [boot_drive], dl

    ; Debug: write '1' to VGA via segment
    mov ax, 0xB800
    mov es, ax
    mov word [es:0x00], 0x0F31

    ; Load stage2
    push ds
    pop es                     ; restore ES=0 for DAP
.load_stage2:
    mov si, dap_stage2
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .loaded

    mov ax, 0xB800
    mov es, ax
    mov word [es:0x02], 0x0F46 ; 'F' = read failed
    push ds
    pop es
.retry:
    xor ah, ah
    int 0x13
    jmp .load_stage2

.loaded:
    mov ax, 0xB800
    mov es, ax
    mov word [es:0x02], 0x0F32 ; '2' = loaded

    ; Verify stage2 first byte (jmp opcode = 0xE9)
    mov al, [0x7E00]
    cmp al, 0xE9
    jne .bad_stage2

    mov word [es:0x04], 0x0F33 ; '3' = verified
    jmp 0x0000:0x7E00

.bad_stage2:
    mov word [es:0x04], 0x0F58 ; 'X' = bad signature
.hang:
    hlt
    jmp .hang

boot_drive db 0

align 8
dap_stage2:
    db 0x10, 0
    dw 10
    dw 0x7E00
    dw 0x0000
    dq 1

times 446-($-$$) db 0
db 0x80, 0, 0, 0, 0x83, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00
times 48 db 0
dw 0xAA55
