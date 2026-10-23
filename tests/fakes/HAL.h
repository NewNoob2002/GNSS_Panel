#pragma once
#include "hc32_fake.h"
#include "mcu_define.h"
#include "platform_debug.h"

namespace HAL {
void HAL_Update();
void Power_Init();
void Battery_Init();
bool Power_RequestOn();
void Power_OnCheck(void (*idle)() = nullptr);
void Power_Shutdown(bool);
void Power_PowerOffMonitor();
bool Power_ShutdownSoftReset();
void Power_Update();
void Power_GetInfo(Power_Monitor_t*);
void WatchDog_Feed();
const char* Power_GetPowerOffCause();
void Power_ProductInit();
void Power_ProductUpdate();
uint16_t Power_AdjustPercent(uint16_t actualPercent);
} // namespace HAL
