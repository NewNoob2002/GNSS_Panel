#pragma once
// PC 模拟器的板级替身：虚拟 GPIO、毫秒时基与固件电源代码用到的少量 MCU 接口，
// 让 Platform/HAL/HAL_Power.cpp 原样在主机上运行。引脚编号沿用 board_pins.h 的 PA0..PH15 命名。
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

enum PinMode_TypeDef { INPUT, OUTPUT, INPUT_PULLUP };

#define LOW         0x0
#define HIGH        0x1
#define SET         1
#define RESET       0
#define RMU_FLAG_SW 1

void pinMode(gpio_pin_t pin, PinMode_TypeDef mode, uint8_t state = LOW);
int digitalRead(gpio_pin_t pin);
void digitalWrite(gpio_pin_t pin, uint32_t value);
void digitalToggle(gpio_pin_t pin);
uint32_t millis();

/* 模拟器没有软件复位：始终按冷启动处理（等待电源键）。 */
inline en_flag_status_t
RMU_GetStatus(int) {
    return RESET;
}

inline void
RMU_ClearStatus() {}

inline void
__WFI() {}
