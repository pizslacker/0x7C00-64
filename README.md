# 0x7C00-64: The iconic BIOS load address (64-bit)

A bare-metal `x86_64` bootloader and freestanding `64-bit` `C` kernel implementation built from scratch without external libraries, runtimes, or modern firmware abstraction layers (`UEFI`).

**0x7C00-64** boots directly on bare silicon or an emulator in `16-bit Real Mode` via the `Master Boot Record` (`MBR`), loads kernel sectors from disk, constructs a `4-level paging hierarchy` (`PML4`), transitions into `64-bit Long Mode` via `PAE` and the EFER Model-Specific Register (`MSR`), and displays text using the memory-mapped VGA frame buffer (`0xB8000`).

---

## Technical Overview

- **MBR Stage (`boot.asm`)**:
  - Initializes segments, registers, and the stack at `0x7C00`.
  - Executes BIOS `INT 0x13, AH=0x02` to load kernel sectors from the boot medium into RAM starting at physical address `0x10000`.
  - Identity maps the lower 2 MB of physical memory using a 2 MB huge page across three tables located from `0x1000` to `0x4000`:
    - **PML4** (Page Map Level 4) at `0x1000`
    - **PDPT** (Page Directory Pointer Table) at `0x2000`
    - **PD** (Page Directory) at `0x3000` with the huge page flag (`0x83`) set
  - Enables **Physical Address Extension (PAE)** via bit 5 of `CR4`.
  - Activates **Long Mode Enable (LME)** by reading and updating the extended feature enable register (`EFER` MSR `0xC0000080`) using `rdmsr` / `wrmsr`.
  - Enables paging and protected mode simultaneously by writing to `CR0` (bits 31 and 0).
  - Performs a far jump using a 64-bit code segment descriptor (`0x00209A0000000000`) to flush the execution pipeline and start 64-bit instruction decoding.
- **Entry Shim (`kernel_entry.asm`)**:
  - Sets up the 64-bit ABI stack frame (`rsp` initialized at `0x90000`), calls the C entry point `main()`, and traps execution in a `cli; hlt` loop upon return.
- **Freestanding C Kernel (`kernel.c`)**:
  - Built with `-ffreestanding`, `-mno-red-zone`, and SIMD disabled (`-mno-sse`).
  - Directly addresses the 16-bit character/attribute memory cells at `0xB8000`.
- **Linker Specification (`linker.ld`)**:
  - Aligns sections (`.text`, `.rodata`, `.data`, `.bss`) on 4 KB page boundaries, mapped linearly starting at address `0x10000`.

---

## Directory Structure

```text
.
├── Makefile          # Build scripts, emulator runners, and debug targets
├── README.md         # Architecture documentation and usage guide
├── LICENSE           # GNU General Public License v3 (GPLv3)
├── boot.asm          # 16-bit MBR bootloader, paging table setup, and Long Mode jump
├── kernel_entry.asm  # 64-bit assembly wrapper calling C main()
├── kernel.c          # Freestanding 64-bit C kernel driving VGA text memory
└── linker.ld         # Linker script locating code at 0x10000
