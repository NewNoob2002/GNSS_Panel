#pragma once
// PC 模拟器的虚拟 GPIO 与共享固件电源时序所需的少量 MCU 接口。
#include <cstdint>

using gpio_pin_t = int16_t;
using en_flag_status_t = int;

// clang-format off
enum : gpio_pin_t {
    PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
    PH0 = 7 * 16, PH1, PH2, SIM_PIN_COUNT = 8 * 16
};
// clang-format on

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

/** 配置虚拟引脚；输出模式采用指定的初始电平。 */
void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t state);
/** 读取虚拟引脚电平；无效引脚返回 GPIO_LOW。 */
int gpio_read(gpio_pin_t pin);
/** 写入虚拟引脚电平；无效引脚不产生影响。 */
void gpio_write(gpio_pin_t pin, uint32_t value);
/** 翻转虚拟引脚电平；无效引脚不产生影响。 */
void gpio_toggle(gpio_pin_t pin);

/** 模拟器按冷启动处理，没有硬件软件复位标志。 */
inline en_flag_status_t
RMU_GetStatus(int) {
    return RESET;
}

/** 模拟器没有待清除的硬件复位标志。 */
inline void
RMU_ClearStatus() {}

/** 模拟器由主循环调度，不休眠等待硬件中断。 */
inline void
__WFI() {}
