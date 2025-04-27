#ifndef NEUIX_FS_H
#define NEUIX_FS_H

#include "neuix_ata.h"
#include "neuix_vga.h"
#include <stdint.h>
#include <stdbool.h>

#define FS_DISK_START_LBA 1
#define FS_MAX_DISK_SECTORS 4096

typedef enum {
    TYPE_FILE,
    TYPE_DIR,
    TYPE_BINARY
} FileType;

typedef struct FileNode {
    char path[256];
    FileType type;
    uint8_t* content;
    uint32_t size;
    struct FileNode* next;
} FileNode;

static FileNode* fs_root = NULL;
static uint8_t fs_disk_cache[512 * FS_MAX_DISK_SECTORS];

static int strlen(const char* s) { int i = 0; while (s[i]) i++; return i; }
static void strcpy(char* dst, const char* src) { while (*src) *dst++ = *src++; *dst = 0; }
static bool streq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}
static void strcat(char* dst, const char* src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static FileType parse_type(const char* s) {
    if (streq(s, "file")) return TYPE_FILE;
    if (streq(s, "dir")) return TYPE_DIR;
    if (streq(s, "binary")) return TYPE_BINARY;
    return TYPE_FILE;
}
static const char* type_to_str(FileType t) {
    switch (t) {
        case TYPE_FILE: return "file";
        case TYPE_DIR: return "dir";
        case TYPE_BINARY: return "binary";
    }
    return "file";
}

// 바이너리 실행용 타입
typedef void (*binary_entry_t)(void);

// 바이너리 파일 실행 함수
typedef void (*binary_entry_t)(void);
static void run_binary(const uint8_t* bin, uint32_t size) {
    uint8_t* target = (uint8_t*)0x100000;
    for (uint32_t i = 0; i < size; i++) {
        target[i] = bin[i];
    }

    extern volatile bool esc_pressed;
    esc_pressed = false;

    __asm__ volatile ("sti");

    binary_entry_t entry = (binary_entry_t)target;
    entry();

    __asm__ volatile ("cli");

    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    vga_write("\n[Exited binary program]\n");
}

// 파일 시스템 초기화
static void fs_init() {
    for (int i = 0; i < FS_MAX_DISK_SECTORS; i++) {
        ata_read_sector(FS_DISK_START_LBA + i, &fs_disk_cache[i * 512]);
    }

    uint8_t* p = fs_disk_cache;
    while (*p) {
        if (*p == '{') {
            p++;
            char path[256] = {0};
            char type[16] = {0};
            char sizebuf[16] = {0};
            char content[4096] = {0};
            int pi = 0, ti = 0, si = 0, ci = 0;

            while (*p && *p != ':' && pi < 255) path[pi++] = *p++;
            if (*p == ':') p++;
            while (*p && *p != ':' && ti < 15) type[ti++] = *p++;
            if (*p == ':') p++;
            while (*p && *p != ':' && si < 15) sizebuf[si++] = *p++;
            if (*p == ':') p++;
            uint32_t size = 0;
            for (int i = 0; i < si; i++) {
                size = size * 10 + (sizebuf[i] - '0');
            }
            for (uint32_t i = 0; i < size && *p && *p != '}'; i++) {
                content[ci++] = *p++;
            }
            if (*p == '}') p++;

            FileNode* node = (FileNode*) malloc(sizeof(FileNode));
            strcpy(node->path, path);
            node->type = parse_type(type);
            node->size = size;
            node->content = 0;
            if (node->type != TYPE_DIR) {
                node->content = (uint8_t*) malloc(size);
                for (uint32_t k = 0; k < size; k++) node->content[k] = content[k];
            }
            node->next = fs_root;
            fs_root = node;
        } else {
            p++;
        }
    }
}

// 파일 시스템 저장
static void fs_save() {
    uint8_t* p = fs_disk_cache;
    FileNode* node = fs_root;
    while (node) {
        *p++ = '{';
        strcat((char*)p, node->path); p += strlen(node->path);
        *p++ = ':';
        strcat((char*)p, type_to_str(node->type)); p += strlen(type_to_str(node->type));
        *p++ = ':';
        char sizebuf[16] = {0};
        int si = 0;
        uint32_t temp = node->size;
        char rev[16];
        int ri = 0;
        if (temp == 0) rev[ri++] = '0';
        while (temp) {
            rev[ri++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = ri-1; i >= 0; i--) sizebuf[si++] = rev[i];
        sizebuf[si] = 0;
        strcat((char*)p, sizebuf); p += strlen(sizebuf);
        *p++ = ':';
        if (node->type != TYPE_DIR && node->content) {
            for (uint32_t i = 0; i < node->size; i++) {
                *p++ = node->content[i];
            }
        }
        *p++ = '}';
    }
    *p = 0;

    int sectors = (p - fs_disk_cache + 511) / 512;
    for (int i = 0; i < sectors; i++) {
        ata_write_sector(FS_DISK_START_LBA + i, &fs_disk_cache[i * 512]);
    }
}

// 파일 리스트 출력
static void fs_list() {
    FileNode* node = fs_root;
    while (node) {
        vga_write(node->path);
        vga_write(" : ");
        vga_write(type_to_str(node->type));
        vga_write("\n");
        node = node->next;
    }
}

// 파일 읽기
static bool fs_read(const char* path, uint8_t* out_buf, uint32_t* out_size) {
    FileNode* node = fs_root;
    while (node) {
        if (streq(node->path, path) && node->type != TYPE_DIR) {
            for (uint32_t i = 0; i < node->size; i++) {
                out_buf[i] = node->content[i];
            }
            *out_size = node->size;
            return true;
        }
        node = node->next;
    }
    return false;
}

// 파일 생성
static void fs_create(const char* path, FileType type, const uint8_t* data, uint32_t size) {
    FileNode* node = (FileNode*) malloc(sizeof(FileNode));
    strcpy(node->path, path);
    node->type = type;
    node->size = size;
    node->content = 0;
    if (type != TYPE_DIR && data) {
        node->content = (uint8_t*) malloc(size);
        for (uint32_t i = 0; i < size; i++) node->content[i] = data[i];
    }
    node->next = fs_root;
    fs_root = node;
    fs_save();
}

#endif // NEUIX_FS_H
