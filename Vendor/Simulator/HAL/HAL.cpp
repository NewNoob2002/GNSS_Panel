#include <HAL.h>

// SDL 发送时读取 Settings 百分比并共用 HAL_Board 的感知亮度表，虚拟板无需发送面板命令。
void
HAL::Display_SetBrightness(uint8_t) {}

// 模拟器主循环：只运行固件的电源时序（HAL_Power.cpp）；电池采样与充电监控由 main.cpp 模拟。

void
HAL::HAL_Init() {
    Power_Init();
}

void
HAL::HAL_Update() {
    Power_PowerOffMonitor();
}
