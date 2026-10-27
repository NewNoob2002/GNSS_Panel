#pragma once

/**
 * @file platform_debug.h
 * @brief Debug and assertion macros for Simulator platform.
 */

#include "Log.h"

#ifndef CORE_DEBUG_PRINTF
#define CORE_DEBUG_PRINTF(...) APP_LOG_D("HAL", __VA_ARGS__)
#endif

#ifndef CORE_ASSERT
#define CORE_ASSERT(expression, message, ...)                                                                          \
    if (!(expression)) {                                                                                               \
        __VA_ARGS__;                                                                                                   \
    }
#endif

#define CORE_ASSERT_FAIL(message) CORE_ASSERT(false, message)

#define ASSERT_GPIO_PIN_VALID(gpio_pin, fn_name, ...)
