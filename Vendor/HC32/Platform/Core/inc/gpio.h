#pragma once
#include "WVariant.h"
#include "addon_gpio.h"
#include "core_debug.h"
#include "core_types.h"


#define Func_GPIO       GPIO_FUNC_0
#define Func_Tim4       GPIO_FUNC_2
#define Func_Tim6       GPIO_FUNC_3
#define Func_Tima0      GPIO_FUNC_4
#define Func_Tima1      GPIO_FUNC_5
#define Func_Tima2      GPIO_FUNC_6
#define Func_Emb        GPIO_FUNC_6
#define Func_Usart_Ck   GPIO_FUNC_7
#define Func_Spi_Nss    GPIO_FUNC_7
#define Func_Qspi       GPIO_FUNC_7
#define Func_Key        GPIO_FUNC_8
#define Func_Sdio       GPIO_FUNC_9
#define Func_I2s        GPIO_FUNC_10
#define Func_UsbF       GPIO_FUNC_10
#define Func_Evnpt      GPIO_FUNC_14
#define Func_Eventout   GPIO_FUNC_15
#define Func_Usart1_Tx  GPIO_FUNC_32
#define Func_Usart3_Tx  GPIO_FUNC_32
#define Func_Usart1_Rx  GPIO_FUNC_33
#define Func_Usart3_Rx  GPIO_FUNC_33
#define Func_Usart1_Rts GPIO_FUNC_34
#define Func_Usart3_Rts GPIO_FUNC_34
#define Func_Usart1_Cts GPIO_FUNC_35
#define Func_Usart3_Cts GPIO_FUNC_35
#define Func_Usart2_Tx  GPIO_FUNC_36
#define Func_Usart4_Tx  GPIO_FUNC_36
#define Func_Usart2_Rx  GPIO_FUNC_37
#define Func_Usart4_Rx  GPIO_FUNC_37
#define Func_Usart2_Rts GPIO_FUNC_38
#define Func_Usart4_Rts GPIO_FUNC_38
#define Func_Usart2_Cts GPIO_FUNC_39
#define Func_Usart4_Cts GPIO_FUNC_39
#define Func_Spi1_Mosi  GPIO_FUNC_40
#define Func_Spi3_Mosi  GPIO_FUNC_40
#define Func_Spi1_Miso  GPIO_FUNC_41
#define Func_Spi3_Miso  GPIO_FUNC_41
#define Func_Spi1_Nss0  GPIO_FUNC_42
#define Func_Spi3_Nss0  GPIO_FUNC_42
#define Func_Spi1_Sck   GPIO_FUNC_43
#define Func_Spi3_Sck   GPIO_FUNC_43
#define Func_Spi2_Mosi  GPIO_FUNC_44
#define Func_Spi4_Mosi  GPIO_FUNC_44
#define Func_Spi2_Miso  GPIO_FUNC_45
#define Func_Spi4_Miso  GPIO_FUNC_45
#define Func_Spi2_Nss0  GPIO_FUNC_46
#define Func_Spi4_Nss0  GPIO_FUNC_46
#define Func_Spi2_Sck   GPIO_FUNC_47
#define Func_Spi4_Sck   GPIO_FUNC_47
#define Func_I2c1_Sda   GPIO_FUNC_48
#define Func_I2c3_Sda   GPIO_FUNC_48
#define Func_I2c1_Scl   GPIO_FUNC_49
#define Func_I2c3_Scl   GPIO_FUNC_49
#define Func_I2c2_Sda   GPIO_FUNC_50
#define Func_Can1_Tx    GPIO_FUNC_50
#define Func_I2c2_Scl   GPIO_FUNC_51
#define Func_Can1_Rx    GPIO_FUNC_51
#define Func_I2s1_Sd    GPIO_FUNC_52
#define Func_I2s3_Sd    GPIO_FUNC_52
#define Func_I2s1_Sdin  GPIO_FUNC_53
#define Func_I2s3_Sdin  GPIO_FUNC_53
#define Func_I2s1_Ws    GPIO_FUNC_54
#define Func_I2s3_Ws    GPIO_FUNC_54
#define Func_I2s1_Ck    GPIO_FUNC_55
#define Func_I2s3_Ck    GPIO_FUNC_55
#define Func_I2s2_Sd    GPIO_FUNC_56
#define Func_I2s4_Sd    GPIO_FUNC_56
#define Func_I2s2_Sdin  GPIO_FUNC_57
#define Func_I2s4_Sdin  GPIO_FUNC_57
#define Func_I2s2_Ws    GPIO_FUNC_58
#define Func_I2s4_Ws    GPIO_FUNC_58
#define Func_I2s2_Ck    GPIO_FUNC_59
#define Func_I2s4_Ck    GPIO_FUNC_59

#ifdef __cplusplus
extern "C" {
#endif

//
// GPIO wrappers for PORT_* functions
//
#define PIN_ARG(gpio_pin) PIN_MAP[gpio_pin].port, 1 << PIN_MAP[gpio_pin].bit_pos

/**
 * @brief GPIO wrapper for PORT_Init
 */
inline int32_t
_GPIO_Init(gpio_pin_t gpio_pin, const stc_gpio_init_t* pstcPortInit) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_Init");
    return GPIO_Init(PIN_ARG(gpio_pin), pstcPortInit);
}

/**
 * @brief GPIO wrapper for PORT_GetConfig
 */
inline int32_t
GPIO_GetConfig(gpio_pin_t gpio_pin, stc_gpio_init_t* pstcPortInit) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_GetConfig");
    return PORT_GetConfig(PIN_ARG(gpio_pin), pstcPortInit);
}

/**
 * @brief GPIO wrapper for PORT_GetBit
 */
inline en_pin_state_t
GPIO_GetBit(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_GetBit");
    return GPIO_ReadInputPins(PIN_ARG(gpio_pin));
}

//    /**
//     * @brief GPIO wrapper for PORT_OE
//     */
//    inline en_result_t GPIO_OE(gpio_pin_t gpio_pin, en_functional_state_t enNewState)
//    {
//        ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_OE");
//        return PORT_OE(PIN_ARG(gpio_pin), enNewState);
//    }

/**
 * @brief GPIO wrapper for PORT_SetBits
 */
inline void
GPIO_SetBits(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_SetBits");
    GPIO_SetPins(PIN_ARG(gpio_pin));
}

/**
 * @brief GPIO wrapper for PORT_ResetBits
 */
inline void
GPIO_ResetBits(gpio_pin_t gpio_pin) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_ResetBits");
    return GPIO_ResetPins(PIN_ARG(gpio_pin));
}

/**
 * @brief GPIO wrapper for PORT_Toggle
 */
inline void
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
inline void
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
inline void
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

/**
 * @brief GPIO wrapper for PORT_GetFunc
 * @param enFuncSelect GPIO pin primary function select
 * @param enSubFunc GPIO pin sub-function enable/disable (subfunction is GPIO output for most pins)
 */
inline int32_t
GPIO_GetFunc(gpio_pin_t gpio_pin, uint16_t* enFuncSelect, en_functional_state_t* enSubFunc) {
    ASSERT_GPIO_PIN_VALID(gpio_pin, "GPIO_GetFunc");
    return PORT_GetFunc(PIN_ARG(gpio_pin), enFuncSelect, enSubFunc);
}

#ifdef __cplusplus
}
#endif
