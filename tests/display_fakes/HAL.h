#pragma once
// 主机测试：AppPlatform 用到的 HAL 接口，由 display_app_tests.cpp 实现。
#include "core_debug.h"

namespace HAL {
void Power_Shutdown(bool completed);
bool Key_PowerPressed();
bool Key_FunctionPressed();
} // namespace HAL

void HAL_NVIC_SystemReset();
