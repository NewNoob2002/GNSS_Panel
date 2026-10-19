#pragma once

inline void
fake_debug_print(const char*, ...) {}

#define CORE_DEBUG_PRINTF(...) fake_debug_print(__VA_ARGS__)
