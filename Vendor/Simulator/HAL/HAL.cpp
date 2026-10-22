#include <HAL.h>

// 模拟器主循环：只运行固件的电源时序（HAL_Power.cpp）；电池采样与充电监控由 main.cpp 模拟。

void
HAL::HAL_Init() {
    Power_Init();
}

void
HAL::HAL_Update() {
    Power_PowerOffMonitor();
}
