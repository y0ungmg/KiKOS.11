BITS 32
global isr_stub_table
extern isr_dispatch

%macro ISRSTUB 1
isr_%1:
    pushad
    push dword %1
    call isr_dispatch
    add esp, 4
    popad
    iretd
%endmacro

%assign i 0
%rep 256
ISRSTUB i
%assign i i+1
%endrep

section .data
align 8
isr_stub_table:

%macro ADDR 1
    dd isr_%1
%endmacro

%assign i 0
%rep 256
ADDR i
%assign i i+1
%endrep
