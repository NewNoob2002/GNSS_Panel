#pragma once
#include <stdint.h>
// 固件 lv_conf.h 的 LVGL 时基（LV_TICK_CUSTOM）：主机测试控制的毫秒时间。
#ifdef __cplusplus
extern "C" {
#endif
uint32_t millis(void);
#ifdef __cplusplus
}
#endif
