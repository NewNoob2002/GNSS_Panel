#ifndef _CORE_DEBUG_H
#define _CORE_DEBUG_H

//#define __CORE_DEBUG

#ifdef __CORE_DEBUG
#include "elog.h"
// allow user to re-define the debug macros with custom ones
// user macros are only active if __CORE_DEBUG is defined
#ifndef CORE_DEBUG_INIT
#define CORE_DEBUG_INIT()
#endif

#ifndef CORE_DEBUG_PRINTF
#define CORE_DEBUG_PRINTF(fmt, ...) log_i(fmt, ##__VA_ARGS__)
#endif

#ifndef CORE_ASSERT
#define CORE_ASSERT(expression, message, ...)                                                                          \
    if (!(expression)) {                                                                                               \
        CORE_DEBUG_PRINTF("CORE_ASSERT:" message " (Line: %d, Function: %s)\n\n", ##__VA_ARGS__, __LINE__,             \
                          __FUNCTION__);                                                                               \
        while (true)                                                                                                   \
            ;                                                                                                          \
    }
#endif
#else // !__CORE_DEBUG
// no debug, dummy macros and user-macros are undefined
#undef CORE_DEBUG_INIT
#undef CORE_DEBUG_PRINTF
#undef CORE_ASSERT
#define CORE_DEBUG_PRINTF(fmt, ...)
#define CORE_DEBUG_INIT()
#define CORE_ASSERT(expression, message, ...)                                                                          \
    if (!(expression)) {                                                                                               \
        __VA_ARGS__;                                                                                                   \
    }
#endif // __CORE_DEBUG

#define CORE_ASSERT_FAIL(message) CORE_ASSERT(false, message)

#include "WVariant.h"
#define ASSERT_GPIO_PIN_VALID(gpio_pin, fn_name, ...)                                                                  \
    // CORE_ASSERT(IS_GPIO_PIN(gpio_pin), "invalid GPIO pin supplied to " fn_name " (Pin: %d)", gpio_pin, ##__VA_ARGS__)

#endif // _CORE_DEBUG_H