#include "lv_port.h"

void
lv_port_init() {
    // 输入不再经 LVGL indev：两键电平由 AppPlatform 交给共享应用（P4App）处理焦点与操作。
    lv_port_disp_init();
}