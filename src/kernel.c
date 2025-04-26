#include "vga.h"

// 엔트리 포인트 (부트로더가 호출하는 함수)
void kernel_main(void) {
    vga_init();  // 화면 초기화
    vga_set_color(COLOR_WHITE, COLOR_BLUE);  // 흰 글씨 + 파란 배경
    vga_write("neuix successfully booted!\n");
    vga_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    vga_write("loading userland.\n");
    userland();

}
