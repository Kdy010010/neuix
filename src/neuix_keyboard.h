#ifndef NEUIX_KEYBOARD_H
#define NEUIX_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "neuix_vga.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEY_BUFFER_SIZE 128

static char input_buffer[KEY_BUFFER_SIZE];
static uint8_t buffer_index = 0;
static bool enter_pressed = false;
volatile bool esc_pressed = false; // <-- ESC 감지 플래그 추가

// US QWERTY 스캔코드 테이블 (0~57)
static const char scancode_table[58] = {
    0,  '\x1B', '1', '2', '3', '4', '5', '6', '7', '8',     // 0x00 - 0x09
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',    // 0x0A - 0x13
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',          // 0x14 - 0x1D
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',      // 0x1E - 0x27
    ';', '\'', '`', 0,  '\\', 'z', 'x', 'c', 'v', 'b',     // 0x28 - 0x31
    'n', 'm', ',', '.', '/', 0,   '*', 0,   ' '            // 0x32 - 0x39
};

// 스캔코드를 문자로 변환
static char scancode_to_char(uint8_t scancode) {
    if (scancode < sizeof(scancode_table)) {
        return scancode_table[scancode];
    }
    return 0;
}

// 키보드 인터럽트 핸들러
void keyboard_isr_handler() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode & 0x80) {
        return; // release는 무시
    }

    if (scancode == 0x01) { // ESC 키
        esc_pressed = true;
        return;
    }

    char c = scancode_to_char(scancode);
    if (c) {
        if (c == '\b') {
            if (buffer_index > 0) {
                buffer_index--;
                input_buffer[buffer_index] = '\0';
                vga_put_char('\b');
                vga_put_char(' ');
                vga_put_char('\b');
            }
        } else {
            if (buffer_index < KEY_BUFFER_SIZE - 1) {
                input_buffer[buffer_index++] = c;
                input_buffer[buffer_index] = '\0';
                vga_put_char(c);

                if (c == '\n') {
                    enter_pressed = true;
                }
            }
        }
    }
}

// 입력 버퍼 가져오기
const char* keyboard_get_buffer() {
    input_buffer[buffer_index] = '\0';
    return input_buffer;
}

// 버퍼 비우기
void keyboard_clear_buffer() {
    buffer_index = 0;
    input_buffer[0] = '\0';
    enter_pressed = false;
    esc_pressed = false;
}

// getchar: 입력될 때까지 대기하고 한 글자 리턴
char getchar() {
    while (true) {
        if (buffer_index > 0) {
            char c = input_buffer[0];
            for (int i = 0; i < buffer_index - 1; i++) {
                input_buffer[i] = input_buffer[i + 1];
            }
            buffer_index--;
            input_buffer[buffer_index] = '\0';
            return c;
        }
    }
}

// getline: 엔터가 눌릴 때까지 입력받아 저장
void getline(char* out_buf, int max_len) {
    keyboard_clear_buffer();
    while (!enter_pressed);
    
    int len = 0;
    for (int i = 0; i < buffer_index && len < max_len - 1; i++) {
        if (input_buffer[i] == '\n') break;
        out_buf[len++] = input_buffer[i];
    }
    out_buf[len] = '\0';
    keyboard_clear_buffer();
}

#endif // NEUIX_KEYBOARD_H
