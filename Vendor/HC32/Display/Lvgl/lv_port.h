#ifndef LV_PORT_H
#define LV_PORT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif
void lv_port_init();

void lv_port_disp_init();
void lv_port_disp_poll();

void lv_port_reset();
#if defined(__CORE_DEBUG) && (defined(__arm__) || defined(__thumb__))
/**
 * @brief 返回 MSP 栈涂色高水位字节数；涂色失败返回零。
 * @details 仅调试 ARM 构建存在，主循环/调试器暂停后调用，禁止 ISR 调用。
 *          覆盖 main 后的栈和异常压栈，不覆盖涂色前的启动过程；同值覆盖可能低估。
 *          当前帧和初始化保留区计入使用量，8 KiB 减返回值即剩余水位。
 */
uint32_t lv_port_debug_stack_used();
#endif
#ifdef __cplusplus
}
#endif

#endif