#ifndef NEUIX_USERLAND_H
#define NEUIX_USERLAND_H

#include "vga.h"
#include "keyboard.h"
#include "io.h"
#include <stdbool.h>
#include <stdint.h>

// 문자열 비교
static bool strcmp(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

static bool strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return false;
    }
    return true;
}

// 문자열 -> 숫자 (16진수 또는 10진수)
static uint16_t parse_number(const char* str) {
    uint16_t result = 0;
    bool hex = false;

    if (str[0] == '0' && str[1] == 'x') {
        hex = true;
        str += 2;
    }

    while (*str) {
        result *= hex ? 16 : 10;
        if (*str >= '0' && *str <= '9') {
            result += *str - '0';
        } else if (hex && *str >= 'a' && *str <= 'f') {
            result += 10 + (*str - 'a');
        } else if (hex && *str >= 'A' && *str <= 'F') {
            result += 10 + (*str - 'A');
        } else {
            break;
        }
        str++;
    }
    return result;
}

// 문자열 출력 숫자
static void print_hex(uint16_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    vga_write("0x");
    for (int i = 12; i >= 0; i -= 4) {
        vga_put_char(hex_chars[(val >> i) & 0xF]);
    }
    vga_write("\n");
}

// 프롬프트 출력
static void shell_prompt() {
    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    vga_write("neux> ");
}

// 명령어 실행
static void shell_execute(const char* cmd) {
    if (cmd[0] == '\0') return;

    if (strcmp(cmd, "help")) {
        vga_write("Commands: help, clear, echo, about, reboot, off, time, inb, inw, outb, outw\n");
    } else if (strcmp(cmd, "clear")) {
        vga_clear();
    } else if (strncmp(cmd, "echo ", 5)) {
        vga_write(cmd + 5);
        vga_write("\n");
    } else if (strcmp(cmd, "about")) {
        vga_write("NEUIX Shell - educational kernel project.\n");
    } else if (strcmp(cmd, "reboot")) {
        vga_write("Rebooting...\n");
        outb(0x64, 0xFE);
        while (1);
    } else if (strcmp(cmd, "off")) {
        vga_write("Shutting down...\n");
        __asm__ volatile ("cli; hlt");
    } else if (strcmp(cmd, "time")) {
        vga_write("Time: [stub] RTC not implemented yet.\n");
    } else if (strncmp(cmd, "inb ", 4)) {
        uint16_t port = parse_number(cmd + 4);
        uint8_t val = inb(port);
        vga_write("Value: ");
        print_hex(val);
    } else if (strncmp(cmd, "inw ", 4)) {
        uint16_t port = parse_number(cmd + 4);
        uint16_t val = inw(port);
        vga_write("Value: ");
        print_hex(val);
    } else if (strncmp(cmd, "outb ", 5)) {
        const char* args = cmd + 5;
        while (*args == ' ') args++;
        uint16_t port = parse_number(args);
        while (*args && *args != ' ') args++;
        while (*args == ' ') args++;
        uint8_t val = (uint8_t)parse_number(args);
        outb(port, val);
        vga_write("outb done\n");
    } else if (strncmp(cmd, "outw ", 5)) {
        const char* args = cmd + 5;
        while (*args == ' ') args++;
        uint16_t port = parse_number(args);
        while (*args && *args != ' ') args++;
        while (*args == ' ') args++;
        uint16_t val = parse_number(args);
        outw(port, val);
        vga_write("outw done\n");
    } else {
        vga_write("Unknown command: ");
        vga_write(cmd);
        vga_write("\n");
    }
}

// 메인 쉘 루프
static void userland() {
    vga_set_color(COLOR_YELLOW, COLOR_BLACK);
    vga_write("\n[NEUIX Shell] Type 'help' for commands.\n");

    char command[128];

    while (1) {
        shell_prompt();
        getline(command, sizeof(command));
        shell_execute(command);
    }
}

#endif // NEUIX_USERLAND_H
