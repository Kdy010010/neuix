#include "neuix_vga.h"
#include "neuix_keyboard.h"
#include "neuix_fs.h"
#include "neuix_userland.h"

void kernel_main() {
    vga_set_color(COLOR_WHITE, COLOR_BLACK);
    vga_clear();
    vga_write("contact to kdywkrrk@gmail.com\n");
    vga_write("notice that your disk can be busted\n");
    vga_write("Neuix 1.2 booted\n");

    fs_init();        // 파일시스템 초기화
    keyboard_init();  // 키보드 초기화 (필요시)

    login();          // 로그인
    userland();       // 쉘 시작

    while (1) {
        __asm__ volatile ("hlt");
    }
}
