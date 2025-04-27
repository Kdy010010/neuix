#ifndef NEUIX_FS_H
#define NEUIX_FS_H

#include "neuix_ata.h"
#include "neuix_vga.h"
#include <stdint.h>
#include <stdbool.h>

#define FS_DISK_START_LBA 1
#define FS_MAX_DISK_SECTORS 4096  // 디스크 공간 4096섹터 사용 (필요시 더 늘릴 수 있음)

typedef enum {
    TYPE_FILE,
    TYPE_DIR,
    TYPE_BINARY
} FileType;

typedef struct FileNode {
    char path[256];     // 경로/파일명
    FileType type;      // file, dir, binary
    uint8_t* content;   // 내용 (file, binary)
    uint32_t size;      // 크기 (bytes)
    struct FileNode* next;
} FileNode;

static FileNode* fs_root = NULL;

// 간단한 malloc 대체 (디스크 캐시용)
static uint8_t fs_disk_cache[512 * FS_MAX_DISK_SECTORS];

// 문자열 조작 헬퍼
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

// 타입 파싱
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

// 파일 시스템 초기화
static void fs_init() {
    // 디스크 읽기
    for (int i = 0; i < FS_MAX_DISK_SECTORS; i++) {
        ata_read_sector(FS_DISK_START_LBA + i, &fs_disk_cache[i * 512]);
    }

    uint8_t* p = fs_disk_cache;
    while (*p) {
        if (*p == '{') {
            p++;
            // path:type:content} 추출
            char path[256] = {0};
            char type[16] = {0};
            char content[2048] = {0};

            int pi = 0, ti = 0, ci = 0;

            // path
            while (*p && *p != ':' && pi < 255) path[pi++] = *p++;
            if (*p == ':') p++;

            // type
            while (*p && *p != ':' && ti < 15) type[ti++] = *p++;
            if (*p == ':') p++;

            // content
            while (*p && *p != '}') content[ci++] = *p++;
            if (*p == '}') p++;

            // 노드 생성
            FileNode* node = (FileNode*) (fs_disk_cache + (uintptr_t)node);
            node = (FileNode*) malloc(sizeof(FileNode));
            strcpy(node->path, path);
            node->type = parse_type(type);
            node->size = ci;
            node->content = 0;
            if (node->type != TYPE_DIR) {
                node->content = (uint8_t*) malloc(ci + 1);
                for (int k = 0; k < ci; k++) node->content[k] = content[k];
                node->content[ci] = 0;
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
        if (node->type != TYPE_DIR && node->content) {
            for (uint32_t i = 0; i < node->size; i++) {
                *p++ = node->content[i];
            }
        }
        *p++ = '}';
    }
    *p = 0;

    // 디스크에 저장
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
static bool fs_read(const char* path, uint8_t* out_buf) {
    FileNode* node = fs_root;
    while (node) {
        if (streq(node->path, path) && node->type != TYPE_DIR) {
            for (uint32_t i = 0; i < node->size; i++) {
                out_buf[i] = node->content[i];
            }
            out_buf[node->size] = 0;
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
    if (type != TYPE_DIR) {
        node->content = (uint8_t*) malloc(size + 1);
        for (uint32_t i = 0; i < size; i++) node->content[i] = data[i];
        node->content[size] = 0;
    }
    node->next = fs_root;
    fs_root = node;
    fs_save();
}

#endif // NEUIX_FS_H
