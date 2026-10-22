#pragma once
// 主机测试：AppPlatform 与真实 HAL_Power.cpp 使用的板级接口。
#include <cstdint>
#include <delay.h>
using gpio_pin_t = int16_t;
using en_flag_status_t = int;

enum gpio_mode_t {
    GPIO_INPUT,
    GPIO_INPUT_PULLUP,
    GPIO_INPUT_ANALOG,
    GPIO_OUTPUT,
    GPIO_OUTPUT_OPEN_DRAIN,
    GPIO_OUTPUT_AF_PP,
    GPIO_OUTPUT_AF_OD
};

enum { GPIO_LOW = 0, GPIO_HIGH = 1 };

#define SET         1
#define RESET       0
#define RMU_FLAG_SW 1

enum { PA0, PA6, PB3 = 19, PB5, PC13, PH2 };

/** 读取虚拟板级引脚的电平。 */
int gpio_read(gpio_pin_t pin);
/** 写入虚拟板级引脚的电平。 */
void gpio_write(gpio_pin_t pin, uint32_t value);
/** 记录初始化电平，供共享电源时序回归使用。 */
void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t value);
/** 翻转虚拟板级引脚的电平。 */
void gpio_toggle(gpio_pin_t pin);
en_flag_status_t RMU_GetStatus(int flag);
void RMU_ClearStatus();

/** 主机没有中断休眠，测试回调负责推进时钟。 */
inline void
__WFI() {}
