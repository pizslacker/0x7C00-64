/*
 * 0x7C00-64 - Bare-Metal x86_64 Bootloader & Minimal C Kernel
 * Licensed under GPLv3
 */

/*
 * 0x7C00-64 - Bare-Metal x86_64 Bootloader & Minimal C Kernel
 * Licensed under GPLv3
 */

#include <stdint.h>
#include <stdbool.h>

#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define RED_ON_BLACK   0x0C

static uint16_t cursor_row = 0;
static uint16_t cursor_col = 0;

/* -----------------------------------------------------------------------------
 * VGA Printing Helpers
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
 * Vendor String (Leaf 0x00000000)
 * EBX, EDX, ECX return a 12-byte ASCII string in that order.
 * -------------------------------------------------------------------------- */
void get_cpu_vendor(char vendor[13]) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);

    // EBX (bytes 0-3)
    vendor[0] = (char)(ebx & 0xFF);
    vendor[1] = (char)((ebx >> 8) & 0xFF);
    vendor[2] = (char)((ebx >> 16) & 0xFF);
    vendor[3] = (char)((ebx >> 24) & 0xFF);

    // EDX (bytes 4-7)
    vendor[4] = (char)(edx & 0xFF);
    vendor[5] = (char)((edx >> 8) & 0xFF);
    vendor[6] = (char)((edx >> 16) & 0xFF);
    vendor[7] = (char)((edx >> 24) & 0xFF);

    // ECX (bytes 8-11)
    vendor[8]  = (char)(ecx & 0xFF);
    vendor[9]  = (char)((ecx >> 8) & 0xFF);
    vendor[10] = (char)((ecx >> 16) & 0xFF);
    vendor[11] = (char)((ecx >> 24) & 0xFF);

    vendor[12] = '\0';
}

/* -----------------------------------------------------------------------------
 * Feature Flags (Leaf 0x00000001)
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

void check_cpu_features(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    print_str("Processor Feature Capabilities:\n", WHITE_ON_BLACK);
    print_feature_status("TSC     (Time Stamp Counter)", (edx & CPUID_FEAT_EDX_TSC) != 0);
    print_feature_status("MSR     (Model Specific Registers)", (edx & CPUID_FEAT_EDX_MSR) != 0);
    print_feature_status("APIC    (Advanced PIC)", (edx & CPUID_FEAT_EDX_APIC) != 0);
    print_feature_status("SSE     (Streaming SIMD Extensions)", (edx & CPUID_FEAT_EDX_SSE) != 0);
    print_feature_status("SSE2    (Streaming SIMD Extensions 2)", (edx & CPUID_FEAT_EDX_SSE2) != 0);
    print_feature_status("SSE3    (Prescott New Instructions)", (ecx & CPUID_FEAT_ECX_SSE3) != 0);
    print_feature_status("VMX     (Virtual Machine Extensions)", (ecx & CPUID_FEAT_ECX_VMX) != 0);
    print_feature_status("AVX     (Advanced Vector Extensions)", (ecx & CPUID_FEAT_ECX_AVX) != 0);
    print_feature_status("RDRAND  (On-Chip Hardware RNG)", (ecx & CPUID_FEAT_ECX_RDRAND) != 0);
}

/* -----------------------------------------------------------------------------
 * Kernel Entry Point
 * -------------------------------------------------------------------------- */
void main(void) {
    clear_screen();

    print_str("0x7C00-64 Boot Success! Long Mode Active.\n\n", WHITE_ON_BLACK);

    char vendor[13];
    get_cpu_vendor(vendor);
    print_str("CPU Vendor: ", WHITE_ON_BLACK);
    print_str(vendor, GREEN_ON_BLACK);
    print_str("\n\n", WHITE_ON_BLACK);

    check_cpu_features();
}