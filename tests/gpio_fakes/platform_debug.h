#pragma once

/**
 * @file platform_debug.h
 * @brief Fake platform debug definitions for gpio tests.
 */

#ifdef __cplusplus
inline void fake_debug_print(const char*, ...) {}
#else
static inline void fake_debug_print(const char* fmt, ...) { (void)fmt; }
#endif

#define CORE_DEBUG_PRINTF(...) fake_debug_print(__VA_ARGS__)
#define ASSERT_GPIO_PIN_VALID(...)
#define CORE_ASSERT_FAIL(...)
