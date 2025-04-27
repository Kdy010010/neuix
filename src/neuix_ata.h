#ifndef NEUIX_ATA_H
#define NEUIX_ATA_H

#include "io.h"
#include <stdint.h>
#include "neuix_vga.h"

// ATA 포트 정의
#define ATA_DATA_PORT       0x1F0
#define ATA_ERROR_PORT      0x1F1
#define ATA_SECTOR_COUNT    0x1F2
#define ATA_LBA_LOW         0x1F3
#define ATA_LBA_MID         0x1F4
#define ATA_LBA_HIGH        0x1F5
#define ATA_DRIVE_SELECT    0x1F6
#define ATA_COMMAND_PORT    0x1F7
#define ATA_STATUS_PORT     0x1F7

// ATA 명령어
#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30

// 상태 비트
#define ATA_STATUS_ERR  (1 << 0)
#define ATA_STATUS_DRQ  (1 << 3)
#define ATA_STATUS_SRV  (1 << 4)
#define ATA_STATUS_DF   (1 << 5)
#define ATA_STATUS_RDY  (1 << 6)
#define ATA_STATUS_BSY  (1 << 7)

// 디스크 기다리기
static void ata_wait() {
    while (inb(ATA_STATUS_PORT) & ATA_STATUS_BSY);
}

// 읽기 완료 기다리기
static bool ata_wait_drq() {
    uint8_t status;
    do {
        status = inb(ATA_STATUS_PORT);
        if (status & ATA_STATUS_ERR) return false;  // 오류 발생
    } while (!(status & ATA_STATUS_DRQ));
    return true;
}

// 하나의 섹터 읽기 (512바이트)
static bool ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait();

    outb(ATA_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0x0F));  // master, LBA
    outb(ATA_SECTOR_COUNT, 1);                            // 1 sector
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND_PORT, ATA_CMD_READ_SECTORS);

    if (!ata_wait_drq()) {
        vga_write("[ATA] Read error.\n");
        return false;
    }

    for (int i = 0; i < 256; i++) {  // 256 words = 512 bytes
        uint16_t data = inw(ATA_DATA_PORT);
        buffer[i * 2 + 0] = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }

    return true;
}

// 하나의 섹터 쓰기 (512바이트)
static bool ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    ata_wait();

    outb(ATA_DRIVE_SELECT, 0xE0 | ((lba >> 24) & 0x0F));  // master, LBA
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND_PORT, ATA_CMD_WRITE_SECTORS);

    if (!ata_wait_drq()) {
        vga_write("[ATA] Write error.\n");
        return false;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t data = (uint16_t)buffer[i * 2] | ((uint16_t)buffer[i * 2 + 1] << 8);
        outw(ATA_DATA_PORT, data);
    }

    return true;
}

#endif // NEUIX_ATA_H
