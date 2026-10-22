// X1 按键：电源键长按关机，功能键双击切换录制（源自 X1_Panel_dev）。
// 功能键按住 2.5 s 强制关机与带屏产品共用（Platform/HAL/HAL_Key.cpp）；
// 开机长按由共用 HAL::Power_PowerOffMonitor() 处理（2 s），这里只处理已开机后的动作。
#include "GpioPin.h"
#include <HAL.h>
#include "SystemState.h"
#include <delay.h>
#include "multi_button.h"

enum : uint8_t { POWER_KEY_ID = 1, FUNCTION_KEY_ID = 2 };

static constexpr GpioPin powerKeyPin(POWER_KEY_PIN);
static constexpr GpioPin functionKeyPin(FUNCTION_KEY_PIN);
static Button PowerKey, FuncKey;

static uint16_t power_key_hold_count;

static void
PowerKey_Pressing_Callback(Button* btn) {
    (void)btn;
    if (!systemInfo.power.panel_power_on) {
        return;
    }
    // LONG_PRESS_HOLD 在长按判定后每个 tick 触发一次，再累计 200 次后请求关机。
    if (++power_key_hold_count >= 200U) {
        power_key_hold_count = 0;
        HAL::Power_Shutdown(false);
    }
}

static void
PowerKey_Release_Callback(Button* btn) {
    (void)btn;
    power_key_hold_count = 0;
}

static void
FuncKey_DoubleClick_Callback(Button* btn) {
    (void)btn;
    CORE_DEBUG_PRINTF("[FuncKey] Double Click");
    State::Request_RecordToggle();
}

static uint8_t
read_button_gpio(uint8_t button_id) {
    switch (button_id) {
        case POWER_KEY_ID: return powerKeyPin.read();
        case FUNCTION_KEY_ID: return functionKeyPin.read();
        default: return 0;
    }
}

void
HAL::Key_ProductInit() {
    // 电源键按下为高电平，功能键按下为低电平。
    button_init(&PowerKey, read_button_gpio, 1, POWER_KEY_ID);
    button_attach(&PowerKey, BTN_LONG_PRESS_HOLD, PowerKey_Pressing_Callback);
    button_attach(&PowerKey, BTN_PRESS_UP, PowerKey_Release_Callback);

    button_init(&FuncKey, read_button_gpio, 0, FUNCTION_KEY_ID);
    button_attach(&FuncKey, BTN_DOUBLE_CLICK, FuncKey_DoubleClick_Callback);

    button_start(&PowerKey);
    button_start(&FuncKey);
}

void
HAL::Key_ProductUpdate() {
    button_ticks();
}
