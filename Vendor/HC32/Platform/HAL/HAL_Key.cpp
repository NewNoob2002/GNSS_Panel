#include "GpioPin.h"
#include "HAL.h"
#include "HoldTimer.h"
#include "SystemState.h"
#include "delay.h"

// 所有产品共用：Function 键按住 2.5 s 强制关机（与旧 ButtonEvent 500 ms 长按 + 10 次 200 ms 重复一致）。
// 单击/双击等操作由产品处理：带屏产品交给共享应用（AppPlatform），X1 见 Key_ProductInit()。
static constexpr GpioPin powerKey(POWER_KEY_PIN);
static constexpr GpioPin functionKey(FUNCTION_KEY_PIN);
static HoldTimer force_shutdown_hold(2500U);

__attribute__((weak)) void
HAL::Key_ProductInit() {}

__attribute__((weak)) void
HAL::Key_ProductUpdate() {}

void
HAL::Key_Init() {
    CORE_DEBUG_PRINTF("KEY: Init");
    powerKey.setMode(GPIO_INPUT_PULLUP);
    functionKey.setMode(GPIO_INPUT_PULLUP);
    Key_ProductInit();
}

void
HAL::Key_Update() {
    if (force_shutdown_hold.update(Key_FunctionPressed(), millis())) {
        CORE_DEBUG_PRINTF("[FuncKey] Force shutdown");
        State::Request_ForcedShutdown();
    }
    Key_ProductUpdate();
}

// Power 键按下为高电平（与 Power_PowerOffMonitor 开机判定一致），Function 键按下为低电平。
bool
HAL::Key_PowerPressed() {
    return powerKey.read();
}

bool
HAL::Key_FunctionPressed() {
    return !functionKey.read();
}
