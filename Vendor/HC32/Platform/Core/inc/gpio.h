#pragma once
#include "pinmap.h"
#include "platform_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/** GPIO 模式配置逐项沿用原数字 IO 实现；AF_OD 保留与 AF_PP 相同的历史配置。 */
typedef enum {
    GPIO_INPUT,
    GPIO_INPUT_PULLUP,
    GPIO_INPUT_ANALOG,
    GPIO_OUTPUT,
    GPIO_OUTPUT_OPEN_DRAIN,
    GPIO_OUTPUT_AF_PP,
    GPIO_OUTPUT_AF_OD
} gpio_mode_t;

enum { GPIO_LOW = 0, GPIO_HIGH = 1 };

/** 有效 pin 为 0..BOARD_NR_GPIO_PINS-1；初始电平随方向一次传入 GPIO_Init，默认低电平由调用方指定。 */
void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t initial_level);
/** 仅 GPIO_HIGH 写高，其余值写低；不改变引脚模式。 */
void gpio_write(gpio_pin_t pin, uint32_t level);
/** 读取输入寄存器，返回 GPIO_LOW 或 GPIO_HIGH。 */
int gpio_read(gpio_pin_t pin);
/** 翻转输出电平，不改变引脚模式。 */
void gpio_toggle(gpio_pin_t pin);

// GPIO wrappers for PORT_* functions
#define PIN_ARG(gpio_pin) PIN_MAP[gpio_pin].port, 1 << PIN_MAP[gpio_pin].bit_pos

/**
 * @brief GPIO wrapper for PORT_Init
 */
static inline int32_t
_GPIO_Init(gpio_pin_t gpio_pin, const stc_gpio_init_t* pstcPortInit) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_Init");
    return GPIO_Init(PIN_ARG(gpio_pin), pstcPortInit);
}

/**
 * @brief GPIO wrapper for PORT_GetBit
 */
static inline en_pin_state_t
GPIO_GetBit(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_GetBit");
    return GPIO_ReadInputPins(PIN_ARG(gpio_pin));
}

/**
 * @brief GPIO wrapper for PORT_SetBits
 */
static inline void
GPIO_SetBits(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_SetBits");
    GPIO_SetPins(PIN_ARG(gpio_pin));
}

/**
 * @brief GPIO wrapper for PORT_ResetBits
 */
static inline void
GPIO_ResetBits(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_ResetBits");
    return GPIO_ResetPins(PIN_ARG(gpio_pin));
}

/**
 * @brief GPIO wrapper for PORT_Toggle
 */
static inline void
GPIO_Toggle(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_Toggle");
    GPIO_TogglePins(PIN_ARG(gpio_pin));
}

#ifdef __cplusplus
/**
 * @brief GPIO wrapper for PORT_SetFunc
 * @param enFuncSelect GPIO pin primary function select
 * @param enSubFunc GPIO pin sub-function enable/disable (subfunction is GPIO output for most pins)
 */
static inline void
GPIO_SetFunction(gpio_pin_t gpio_pin, uint16_t enFuncSelect, en_functional_state_t enSubFunc = DISABLE,
                 uint8_t u8SubFunc = 0) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_SetFunc");
    GPIO_SetFunc(PIN_ARG(gpio_pin), enFuncSelect);
    if (enSubFunc == ENABLE) {
        if (u8SubFunc == 0) {
            CORE_DEBUG_PRINTF("Enabled SubFunction But not set SubFunction Sel");
        }
    }
}
#else
static inline void
GPIO_SetFunction(gpio_pin_t gpio_pin, uint16_t enFuncSelect, en_functional_state_t enSubFunc, uint8_t u8SubFunc) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_SetFunc");
    GPIO_SetFunc(PIN_ARG(gpio_pin), enFuncSelect);
    if (enSubFunc == ENABLE) {
        if (u8SubFunc == 0) {
            CORE_DEBUG_PRINTF("Enabled SubFunction But not set SubFunction Sel");
        }
    }
}
#endif

#ifdef __cplusplus
}
#endif
