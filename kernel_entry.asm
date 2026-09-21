; 0X7C00-64 - Bare-Metal x86_64 Bootloader & Minimal C Kernel
; Licensed under GPLv3

[bits 64]
[extern main]

global _start
_start:
    call main
.halt:
    cli
    hlt
    jmp .halt