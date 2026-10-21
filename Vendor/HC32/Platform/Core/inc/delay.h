#ifndef DELAY_h
#define DELAY_h
#include <stdint.h>
#include "../../Drivers/inc/hc32_ll.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief 返回上电以来的毫秒数，取自 HAL_GetTick() 的 SysTick 计数。
 *
 * 约 49.7 天回绕一次；比较时间间隔请用无符号减法（now - start）。
 */
extern uint32_t millis(void);

/**
 * \brief 返回上电以来的微秒数，由毫秒节拍加 SysTick 当前计数换算，可在中断中调用。
 *
 * 约 71.6 分钟回绕一次；比较时间间隔请用无符号减法。
 */
extern uint32_t micros(void);

extern void delay_ms(uint32_t ms);

extern void delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif