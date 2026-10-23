#pragma once

/** 主机测试中不输出固件调试信息。 */
inline void
fake_debug_print(const char*, ...) {}

#define CORE_DEBUG_PRINTF(...) fake_debug_print(__VA_ARGS__)
#define ASSERT_GPIO_PIN_VALID(...)
#define CORE_ASSERT_FAIL(...)
