// io.h – Neuix OS 포트 입출력 유틸리티

#ifndef NEUIX_IO_H
#define NEUIX_IO_H

#include <stdint.h>

// 바이트 (8비트) 출력
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// 바이트 (8비트) 입력
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 워드 (16비트) 출력
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// 워드 (16비트) 입력
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif // NEUIX_IO_H
