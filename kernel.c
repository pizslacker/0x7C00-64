/*
 * 0x7C00-64 - Bare-Metal x86_64 Bootloader & Minimal C Kernel
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdbool.h>

#define VGA_ADDRESS     0xB8000
#define WHITE_ON_BLACK  0x0F
#define GREEN_ON_BLACK  0x0A
#define RED_ON_BLACK    0x0C
#define CYAN_ON_BLACK   0x0B

static uint16_t cursor_row = 0;
static uint16_t cursor_col = 0;

/* -----------------------------------------------------------------------------
 * VGA Console Helpers
 * -------------------------------------------------------------------------- */
void clear_screen(void) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_ADDRESS;
    uint16_t blank = (WHITE_ON_BLACK << 8) | ' ';
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = blank;
    }
    cursor_row = 0;
    cursor_col = 0;
}

void print_char(char c, uint8_t color) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        return;
    }

    volatile uint16_t *vga = (volatile uint16_t *)VGA_ADDRESS;
    int offset = cursor_row * 80 + cursor_col;
    vga[offset] = (color << 8) | (uint8_t)c;

    cursor_col++;
    if (cursor_col >= 80) {
        cursor_col = 0;
        cursor_row++;
    }
}

void print_str(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i], color);
    }
}

void print_feature_status(const char *name, bool supported) {
    print_str("  [", WHITE_ON_BLACK);
    if (supported) {
        print_str("OK", GREEN_ON_BLACK);
    } else {
        print_str("--", RED_ON_BLACK);
    }
    print_str("] ", WHITE_ON_BLACK);
    print_str(name, WHITE_ON_BLACK);
    print_str("\n", WHITE_ON_BLACK);
}

/* -----------------------------------------------------------------------------
 * CPUID Inline Assembly Wrapper
 * -------------------------------------------------------------------------- */
static inline void cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a" (*eax),
          "=b" (*ebx),
          "=c" (*ecx),
          "=d" (*edx)
        : "a" (leaf),
          "c" (subleaf)
    );
}

/* -----------------------------------------------------------------------------
 * Leaf 0: Vendor String & Maximum Basic Leaf
 * -------------------------------------------------------------------------- */
uint32_t get_cpu_vendor(char vendor[13]) {
    uint32_t max_leaf, ebx, ecx, edx;
    cpuid(0, 0, &max_leaf, &ebx, &ecx, &edx);

    // ASCII byte order returned in: EBX -> EDX -> ECX
    vendor[0]  = (char)(ebx & 0xFF);
    vendor[1]  = (char)((ebx >> 8) & 0xFF);
    vendor[2]  = (char)((ebx >> 16) & 0xFF);
    vendor[3]  = (char)((ebx >> 24) & 0xFF);

    vendor[4]  = (char)(edx & 0xFF);
    vendor[5]  = (char)((edx >> 8) & 0xFF);
    vendor[6]  = (char)((edx >> 16) & 0xFF);
    vendor[7]  = (char)((edx >> 24) & 0xFF);

    vendor[8]  = (char)(ecx & 0xFF);
    vendor[9]  = (char)((ecx >> 8) & 0xFF);
    vendor[10] = (char)((ecx >> 16) & 0xFF);
    vendor[11] = (char)((ecx >> 24) & 0xFF);

    vendor[12] = '\0';
    return max_leaf;
}

/* -----------------------------------------------------------------------------
 * Leaf 1: Standard Feature Flags
 * -------------------------------------------------------------------------- */
// EDX register flags
#define CPUID_FEAT_EDX_TSC       (1 << 4)
#define CPUID_FEAT_EDX_MSR       (1 << 5)
#define CPUID_FEAT_EDX_APIC      (1 << 9)
#define CPUID_FEAT_EDX_SSE       (1 << 25)
#define CPUID_FEAT_EDX_SSE2      (1 << 26)

// ECX register flags
#define CPUID_FEAT_ECX_SSE3      (1 << 0)
#define CPUID_FEAT_ECX_VMX       (1 << 5)
#define CPUID_FEAT_ECX_AVX       (1 << 28)
#define CPUID_FEAT_ECX_RDRAND    (1 << 30)

void check_leaf1_features(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    print_str("Standard Features (Leaf 1):\n", CYAN_ON_BLACK);
    print_feature_status("TSC    (Time Stamp Counter)",        (edx & CPUID_FEAT_EDX_TSC) != 0);
    print_feature_status("MSR    (Model-Specific Registers)",  (edx & CPUID_FEAT_EDX_MSR) != 0);
    print_feature_status("APIC   (Advanced PIC)",              (edx & CPUID_FEAT_EDX_APIC) != 0);
    print_feature_status("SSE    (Streaming SIMD Ext)",        (edx & CPUID_FEAT_EDX_SSE) != 0);
    print_feature_status("SSE2   (Streaming SIMD Ext 2)",      (edx & CPUID_FEAT_EDX_SSE2) != 0);
    print_feature_status("SSE3   (Prescott New Instructions)", (ecx & CPUID_FEAT_ECX_SSE3) != 0);
    print_feature_status("VMX    (Hardware Virtualization)",   (ecx & CPUID_FEAT_ECX_VMX) != 0);
    print_feature_status("AVX    (Advanced Vector Ext)",       (ecx & CPUID_FEAT_ECX_AVX) != 0);
    print_feature_status("RDRAND (On-Chip Hardware RNG)",      (ecx & CPUID_FEAT_ECX_RDRAND) != 0);
}

/* -----------------------------------------------------------------------------
 * Leaf 7, Subleaf 0: Extended Feature Flags
 * -------------------------------------------------------------------------- */
// EBX register flags
#define CPUID_LEAF7_EBX_FSGSBASE (1 << 0)
#define CPUID_LEAF7_EBX_AVX2     (1 << 5)
#define CPUID_LEAF7_EBX_SMEP     (1 << 7)
#define CPUID_LEAF7_EBX_ERMS     (1 << 9)   // Enhanced REP MOVSB/STOSB
#define CPUID_LEAF7_EBX_INVPCID  (1 << 10)
#define CPUID_LEAF7_EBX_SMAP     (1 << 20)

// ECX register flags
#define CPUID_LEAF7_ECX_UMIP     (1 << 2)   // User-Mode Instruction Prevention
#define CPUID_LEAF7_ECX_LA57     (1 << 16)  // 5-Level Paging

void check_leaf7_features(uint32_t max_leaf) {
    print_str("\nExtended Features (Leaf 7, Subleaf 0):\n", CYAN_ON_BLACK);

    if (max_leaf < 7) {
        print_str("  Leaf 7 not supported by CPU (max leaf = ", RED_ON_BLACK);
        print_char('0' + (char)max_leaf, RED_ON_BLACK);
        print_str(")\n", RED_ON_BLACK);
        return;
    }

    uint32_t eax, ebx, ecx, edx;
    cpuid(7, 0, &eax, &ebx, &ecx, &edx);

    print_feature_status("AVX2     (Advanced Vector Ext 2)",    (ebx & CPUID_LEAF7_EBX_AVX2) != 0);
    print_feature_status("SMEP     (Supervisor Exec Prevent)",   (ebx & CPUID_LEAF7_EBX_SMEP) != 0);
    print_feature_status("SMAP     (Supervisor Access Prevent)", (ebx & CPUID_LEAF7_EBX_SMAP) != 0);
    print_feature_status("FSGSBASE (Direct FS/GS Base Access)",  (ebx & CPUID_LEAF7_EBX_FSGSBASE) != 0);
    print_feature_status("ERMS     (Enhanced REP MOVSB/STOSB)",  (ebx & CPUID_LEAF7_EBX_ERMS) != 0);
    print_feature_status("UMIP     (User Instruction Prevent)",  (ecx & CPUID_LEAF7_ECX_UMIP) != 0);
}

/* -----------------------------------------------------------------------------
 * Kernel Entry Point
 * -------------------------------------------------------------------------- */
void main(void) {
    clear_screen();

    print_str("0x7C00-64 Bootloader -> 64-bit Long Mode Active\n", WHITE_ON_BLACK);
    print_str("-----------------------------------------------\n", WHITE_ON_BLACK);

    char vendor[13];
    uint32_t max_leaf = get_cpu_vendor(vendor);

    print_str("CPU Vendor: ", WHITE_ON_BLACK);
    print_str(vendor, GREEN_ON_BLACK);
    print_str("\n\n", WHITE_ON_BLACK);

    check_leaf1_features();
    check_leaf7_features(max_leaf);
}