#pragma once
// 主机测试：AppPlatform 与真实 HAL_Power.cpp 使用的板级接口。
#include <cstdint>
#include "delay.h"
#define HIGH 1
#define LOW  0
#define SET         1
#define RESET       0
#define RMU_FLAG_SW 1
#define OUTPUT      1
using en_flag_status_t = int;

enum { PA0, PA6, PB3 = 19, PB5, PC13, PH2 };
int digitalRead(int pin);
void digitalWrite(int pin, int value);
void pinMode(int pin, int mode, int value = LOW);
en_flag_status_t RMU_GetStatus(int flag);
void RMU_ClearStatus();

/** The host has no interrupt sleep; test callbacks advance the clock. */
inline void
__WFI() {}
