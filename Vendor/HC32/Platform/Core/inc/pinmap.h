/*
 * Copyright (c) 2015 Arduino LLC. All right reserved.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * 引脚编号与映射迁自 Arduino/Core/variants/hc32f460xx/variant.h 和原板级类型头。
 */
#pragma once
#include <stdint.h>
#include "hc32_ll.h"

typedef int16_t gpio_pin_t;

// clang-format off
typedef enum {
    PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
#ifdef GPIO_D
    PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
#endif
#ifdef GPIO_E
    PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
#endif
    PH0, PH1, PH2, PIN_MAX
} Pin_TypeDef;
// clang-format on

#define BOARD_NR_GPIO_PINS PIN_MAX

typedef struct pin_info_t {
    const uint8_t bit_pos : 5;
    const uint8_t port    : 3;
} pin_info_t;

#ifdef __cplusplus
extern "C" {
#endif
extern const pin_info_t PIN_MAP[BOARD_NR_GPIO_PINS];
#ifdef __cplusplus
}
#endif
