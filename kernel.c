/*
 * 0x7C00-64 - Bare-Metal x86_64 Bootloader & Minimal C Kernel
 * Licensed under GPLv3
 */

#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F

void clear_screen(void) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_ADDRESS;
    uint16_t blank = (WHITE_ON_BLACK << 8) | ' ';

    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = blank;
    }
}

void print_string(const char *str, int row, int col) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_ADDRESS;
    int offset = row * 80 + col;

    for (int i = 0; str[i] != '\0'; i++) {
        vga[offset++] = (WHITE_ON_BLACK << 8) | (uint8_t)str[i];
    }
}

void main(void) {
    clear_screen();
    print_string("Hello, 64-bit World!", 12, 30);
}