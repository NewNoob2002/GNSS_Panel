#include "HAL.h"

void
HAL::HAL_Init() {
    Power_Init();
}

void
HAL::HAL_Update() {
    Power_PowerOffMonitor();
}
