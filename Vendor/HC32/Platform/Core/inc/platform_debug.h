#ifndef PLATFORM_DEBUG_H
#define PLATFORM_DEBUG_H
#include <stdbool.h>

//#define __CORE_DEBUG

#ifdef __CORE_DEBUG
#include "elog.h"
// allow user to re-define the debug macros with custom ones
// user macros are only active if __CORE_DEBUG is defined
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
#undef CORE_DEBUG_PRINTF
#undef CORE_ASSERT
#define CORE_DEBUG_PRINTF(fmt, ...)
#define CORE_ASSERT(expression, message, ...)                                                                          \
    if (!(expression)) {                                                                                               \
        __VA_ARGS__;                                                                                                   \
    }
#endif // __CORE_DEBUG

#define CORE_ASSERT_FAIL(message) CORE_ASSERT(false, message)

#define ASSERT_GPIO_PIN_VALID(gpio_pin, fn_name, ...)

#endif // PLATFORM_DEBUG_H