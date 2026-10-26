#pragma once
// 主机测试：AppPlatform 与 HAL_Power.cpp 使用的 HAL 接口，板级替身由 display_app_tests.cpp 实现。
#include <gpio.h>
#include <platform_debug.h>
#include "mcu_config.h"

namespace HAL {
void HAL_Update();
void Power_Init();
void Power_ProductInit();
void Power_OnCheck(void (*idle)() = nullptr);
bool Power_RequestOn();
void Power_PowerOffMonitor();
bool Power_ShutdownSoftReset();
const char* Power_GetPowerOffCause();
void Power_Shutdown(bool completed);
bool Key_PowerPressed();
bool Key_FunctionPressed();
bool I2C_ProbeDevice(unsigned char address);
void Display_SetBrightness(uint8_t value);
} // namespace HAL

void HAL_NVIC_SystemReset();
