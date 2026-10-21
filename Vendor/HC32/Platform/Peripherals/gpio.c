#include "gpio.h"

// 配置语义迁自原 Arduino 兼容核心的 wiring_digital.cpp（pinMode/digitalWrite 等）；GPIO_Init 将电平与方向一起写入 PCR。
void
gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t initial_level) {
    ASSERT_GPIO_PIN_VALID(pin, "gpio_set_mode");
    stc_gpio_init_t config;
    (void)GPIO_StructInit(&config);
    switch (mode) {
        case GPIO_INPUT: config.u16PinDir = PIN_DIR_IN; break;
        case GPIO_INPUT_PULLUP:
            config.u16PinDir = PIN_DIR_IN;
            config.u16PullUp = PIN_PU_ON;
            break;
        case GPIO_INPUT_ANALOG:
            config.u16PinDir = PIN_DIR_IN;
            config.u16PinAttr = PIN_ATTR_ANALOG;
            break;
        case GPIO_OUTPUT:
            config.u16PinDrv = PIN_HIGH_DRV;
            config.u16PinDir = PIN_DIR_OUT;
            config.u16PullUp = PIN_PU_ON;
            break;
        case GPIO_OUTPUT_OPEN_DRAIN:
            config.u16PinDir = PIN_DIR_OUT;
            config.u16PinOutputType = PIN_OUT_TYPE_NMOS;
            break;
        case GPIO_OUTPUT_AF_PP:
        case GPIO_OUTPUT_AF_OD: config.u16PinDrv = PIN_HIGH_DRV; break;
        default: CORE_ASSERT_FAIL("gpio_set_mode: invalid pin mode"); return;
    }
    config.u16PinState = initial_level;
    _GPIO_Init(pin, &config);
}

void
gpio_write(gpio_pin_t pin, uint32_t level) {
    ASSERT_GPIO_PIN_VALID(pin, "gpio_write");
    if (level == GPIO_HIGH) {
        GPIO_SetBits(pin);
    } else {
        GPIO_ResetBits(pin);
    }
}

int
gpio_read(gpio_pin_t pin) {
    ASSERT_GPIO_PIN_VALID(pin, "gpio_read");
    return GPIO_GetBit(pin) ? GPIO_HIGH : GPIO_LOW;
}

void
gpio_toggle(gpio_pin_t pin) {
    ASSERT_GPIO_PIN_VALID(pin, "gpio_toggle");
    GPIO_Toggle(pin);
}
