#pragma once
#include "hc32_fake.h"
#include "pinmap.h"

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

/** 模拟模式初始化，记录调用方传入的初始电平。 */
inline void
gpio_set_mode(gpio_pin_t pin, gpio_mode_t, uint8_t level) {
    fake_pins[pin] = level;
}

/** 返回虚拟引脚的当前电平。 */
inline int
gpio_read(gpio_pin_t pin) {
    return fake_pins[pin];
}

/** 按原驱动语义，仅 GPIO_HIGH 写高。 */
inline void
gpio_write(gpio_pin_t pin, uint32_t level) {
    fake_pins[pin] = level == GPIO_HIGH;
}

/** 翻转虚拟引脚电平。 */
inline void
gpio_toggle(gpio_pin_t pin) {
    fake_pins[pin] ^= 1;
}
