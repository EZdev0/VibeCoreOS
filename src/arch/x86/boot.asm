; VibeCore OS - x86 Bootloader/Stub
; Simple Multiboot compliant boot stub

section .multiboot
align 4
    dd 0x1BADB002             ; Magic number
    dd 0x00                   ; Flags
    dd -(0x1BADB002 + 0x00)   ; Checksum

section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top

    ; Push multiboot info
    push ebx
    push eax

    ; Call kernel
    call kernel_main

    ; Halt if kernel returns
.halt:
    cli
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB stack
stack_top:
