# 0x7C00-64: The iconic BIOS load address (64-bit)

A bare-metal `x86_64` bootloader and freestanding `64-bit` `C` kernel implementation built from scratch without external libraries, runtimes, or modern firmware abstraction layers (`UEFI`).

**0x7C00-64** boots directly on bare silicon or an emulator (`QEMU`) in `16-bit Real Mode` via the `Master Boot Record` (`MBR`), loads kernel sectors from disk, constructs a `4-level paging hierarchy` (`PML4`), transitions into `64-bit Long Mode` via `PAE` and the EFER `Model-Specific Register` (`MSR`), and displays text using the memory-mapped VGA frame buffer (`0xB8000`).

---

## Technical Overview

- **MBR Stage (`boot.asm`)**:
  - Initializes segments, registers, and the stack at [`0x7C00`](https://stackoverflow.com/questions/51995987/bios-and-address-0x07c00).
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
├── LICENSE           # GPLv3 license
├── Makefile          # Build scripts, emulator runners, and debug targets
├── README.md         # Architecture documentation and usage guide
├── LICENSE           # GNU General Public License v3 (GPLv3)
├── boot.asm          # 16-bit MBR bootloader, paging table setup, and Long Mode jump
├── kernel.c          # Freestanding 64-bit C kernel driving VGA text memory
├── kernel_entry.asm  # 64-bit assembly wrapper calling C main()
└── linker.ld         # Linker script locating code at 0x10000
```

### Prerequisites
Building requires standard x86_64 development utilities, NASM, and QEMU.

#### Debian / Ubuntu / Mint
```Bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86
```

#### Arch Linux
```Bash
sudo pacman -S base-devel nasm qemu-system-x86
```

#### Fedora
```Bash
sudo dnf install gcc nasm qemu-system-x86
```

### Building and Running
1. Build the Disk Image

Compile all assembly shims and C files into flat binaries, concatenate them, and pad the resulting image:

```Bash
make
```

Output files in build/:
```text
build/boot.bin: 512-byte MBR boot sector terminated by 0xAA55.

build/kernel.bin: 64-bit flat kernel binary.

build/0x7C00-64.img: 16 KB raw bootable image.
```

2. Boot in QEMU

Launch QEMU with the raw image attached as a drive:

```Bash
make run
```

3. Debug with GDB

To inspect the boot sequence and transition from real mode to long mode step by step:

Launch QEMU with a halted CPU listening on port 1234:

```Bash
make debug
```

In a separate terminal, connect GDB:

```Bash
gdb -ex "target remote localhost:1234" \
    -ex "set architecture i8086" \
    -ex "break *0x7C00" \
    -ex "continue"
```
(Switch architectures via set architecture i386:x86-64 after the long mode transition).

4. Clean Build Files
```Bash
make clean
```

### Memory Map
| Physical Address Range | Usage / Mapped Region |
|------------------------|--------------------------------
| `0x00000 - 0x003FF` | Real Mode Interrupt Vector Table (IVT) |
| `0x00400 - 0x004FF` | BIOS Data Area (BDA) |
| `0x01000 - 0x01FFF` | Page Map Level 4 (PML4) |
| `0x02000 - 0x02FFF` | Page Directory Pointer Table (PDPT) |
| `0x03000 - 0x03FFF` | Page Directory (PD) with 2MB Identity Page |
| `0x07C00 - 0x07DFF` | MBR Bootloader Binary (boot.bin) |
| `0x10000 - 0x13FFF` | Loaded 64-Bit C Kernel (kernel.bin) |
| `0x80000 - 0x90000` | 64-bit Stack Space (Top at 0x90000) |
| `0xB8000 - 0xB8FA0` | Color Text VGA Buffer (80 columns × 25 rows) |

### License
This project is open-source software licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for terms and conditions.
