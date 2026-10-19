#pragma once
#include "core_debug.h"
#include "hc32_fake.h"
#include "mcu_define.h"

namespace HAL {
void HAL_Update();
void Power_Init();
void Power_OnCheck(void (*idle)() = nullptr);
void Power_Shutdown(bool);
void Power_PowerOffMonitor();
bool Power_ShutdownEnsure();
bool Power_ShutdownForce();
bool Power_ShutdownLinux();
bool Power_ShutdownLowBattery();
bool Power_ShutdownSoftReset();
void Power_Update();
void Power_GetInfo(Power_Monitor_t*);
void Power_EnableCharger(BatteryInfo_t*);
void Power_DisableCharger(BatteryInfo_t*);
void WatchDog_Feed();
const char* Power_GetPowerOffCause();
void Power_ProductInit();
void Power_ProductUpdate();
uint16_t Power_AdjustPercent(uint16_t actualPercent);
} // namespace HAL

bool chagrer_begin(pBatteryInfo_t);
void charger_update(pBatteryInfo_t);
void checkBatteryInfo(pBatteryInfo_t);
void BatteryTemp_Monitor(pBatteryInfo_t);
void Charger_Control_GPIO_Init();
void USB_Switch_GPIO_Init();
void USB_Switch_GPIO_Control(uint8_t);
void Charge_Enable_Switch(uint8_t);
void Charge_Current_Select(uint16_t);
void Charger_Control_Monitor(BatteryInfo_t*);
