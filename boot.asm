; SectorZero - Bare-Metal x86_64 Bootloader & Minimal C Kernel
; Licensed under GPLv3

[org 0x7C00]
[bits 16]

KERNEL_OFFSET equ 0x10000       ; Load kernel at 64KB mark

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [BOOT_DRIVE], dl

    ; Read 20 sectors into ES:BX (0x0000:0x10000)
    ; Since 0x10000 > 64KB, set ES = 0x1000, BX = 0x0000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 20                  ; Sectors to read
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; Reset ES back to 0
    xor ax, ax
    mov es, ax

    ; -------------------------------------------------------------------------
    ; Build 4-Level Identity Paging (First 2MB)
    ; Placed at 0x1000:
    ;   0x1000: PML4
    ;   0x2000: PDPT
    ;   0x3000: PD (Page Directory with 2MB huge page)
    ; -------------------------------------------------------------------------
    ; Clear 12KB for tables (0x1000 to 0x4000)
    mov di, 0x1000
    xor eax, eax
    mov cx, 3072                ; 3072 * 4 bytes = 12288 bytes
    rep stosd

    ; PML4[0] -> points to PDPT at 0x2000 (Present | Writable = 0x03)
    mov dword [0x1000], 0x2003

    ; PDPT[0] -> points to PD at 0x3000 (Present | Writable = 0x03)
    mov dword [0x2000], 0x3003

    ; PD[0] -> Identity maps first 2MB using a 2MB huge page
    ; (Present | Writable | HugePage (bit 7) = 0x83)
    mov dword [0x3000], 0x00000083

    ; -------------------------------------------------------------------------
    ; Enter Protected Mode first
    ; -------------------------------------------------------------------------
    cli
    lgdt [gdt64_descriptor]

    ; Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load CR3 with PML4 base address
    mov eax, 0x1000
    mov cr3, eax

    ; Enable Long Mode in EFER MSR (0xC0000080), Bit 8 = LME
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging (Bit 31) and Protected Mode (Bit 0) in CR0
    mov eax, cr0
    or eax, (1 << 31) | (1 << 0)
    mov cr0, eax

    ; Far jump into 64-bit code segment
    jmp CODE_SEG:init_lm

disk_error:
    jmp $

; -----------------------------------------------------------------------------
; 64-Bit Global Descriptor Table (GDT)
; -----------------------------------------------------------------------------
align 16
gdt64_start:
    dq 0x0000000000000000       ; Null descriptor

gdt64_code:
    ; Base/Limit ignored in 64-bit. Present=1, Ring 0, Exec/Read=1, LongMode=1 (bit 53)
    dq 0x00209A0000000000

gdt64_data:
    ; Present=1, Ring 0, Read/Write=1
    dq 0x0000920000000000
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

CODE_SEG equ gdt64_code - gdt64_start
DATA_SEG equ gdt64_data - gdt64_start

; -----------------------------------------------------------------------------
; 64-Bit Long Mode Entry
; -----------------------------------------------------------------------------
[bits 64]
init_lm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, 0x90000            ; Set 64-bit stack pointer
    mov rax, KERNEL_OFFSET
    jmp rax                     ; Jump to 64-bit kernel

BOOT_DRIVE: db 0

times 510 - ($ - $$) db 0
dw 0xAA55