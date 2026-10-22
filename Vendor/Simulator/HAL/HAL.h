#pragma once
// 模拟器的 HAL 声明：电源时序直接编译固件的 Platform/HAL/HAL_Power.cpp（经 CMake 强制包含本文件，
// 并屏蔽固件 HAL.h 的硬件依赖）；电池、充电与按键由 main.cpp 模拟。
#define __HAL_H // 固件 HAL.h 的包含保护：在模拟器中不展开

#include <cstdio>
#include "mcu_config.h"

#define CORE_DEBUG_PRINTF(fmt, ...) std::printf("[HAL] " fmt "\n", ##__VA_ARGS__)

namespace HAL {
void HAL_Init();
void HAL_Update();

/* Power（Platform/HAL/HAL_Power.cpp） */
void Power_Init();
bool Power_RequestOn();
void Power_OnCheck(void (*idle)() = nullptr);
void Power_Shutdown(bool en);
void Power_PowerOffMonitor();
bool Power_ShutdownSoftReset();
const char* Power_GetPowerOffCause();
void Power_ProductInit();

/* I2C 器件探测（SystemLoading 自检）：按虚拟主板的器件在线状态应答 */
bool I2C_ProbeDevice(uint8_t address);

/* 模拟器输入：Power 键电平（Enter / 鼠标中键按住） */
void Sim_SetPowerKey(bool pressed);
} // namespace HAL
