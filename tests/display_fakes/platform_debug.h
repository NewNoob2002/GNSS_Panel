#pragma once

inline void
fake_debug_print(const char*, ...) {}

#define CORE_DEBUG_PRINTF(...) fake_debug_print(__VA_ARGS__)
#define ASSERT_GPIO_PIN_VALID(...)
#define CORE_ASSERT_FAIL(...)
#define LOG_E(...)
#define LOG_W(...)
#define LOG_I(...)
#define LOG_D(...)
#define LOG_RAW(...)
