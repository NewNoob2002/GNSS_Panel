// X1 按键：电源键长按关机，功能键双击切换录制、长按强制关机（源自 X1_Panel_dev）。
// 强制关机时长与 X1Pro 一致：ButtonEvent 500 ms 长按 + 10 次 200 ms 重复 = 2.5 s。
// 开机长按由共用 HAL::Power_PowerOffMonitor() 处理（2 s），这里只处理已开机后的动作。
#include "Arduino.h"
#include "HAL.h"
#include "multi_button.h"

enum : uint8_t { POWER_KEY_ID = 1, FUNCTION_KEY_ID = 2 };

static Button PowerKey, FuncKey;
static constexpr uint32_t FORCE_SHUTDOWN_HOLD_MS = 2500U;

static uint16_t power_key_hold_count;
static uint32_t func_key_pressed_ms;
static bool force_shutdown_sent;

static void
PowerKey_Pressing_Callback(Button* btn) {
    (void)btn;
    if (!systemInfo.powerMonitor.panel_power_on) {
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
    systemInfo.recordInfo.record_op = 1;
    systemInfo.recordInfo.record_status =
        systemInfo.recordInfo.record_status == On_Off_Status_ON ? On_Off_Status_OFF : On_Off_Status_ON;
}

static void
FuncKey_PressDown_Callback(Button* btn) {
    (void)btn;
    func_key_pressed_ms = millis();
    force_shutdown_sent = false;
}

// LONG_PRESS_HOLD 在长按判定后每个 tick 触发；按住时长按 millis() 计算，不受 tick 周期影响。
// 旧版用 LONG_PRESS_START（每次按下只触发一次）计数 10 次，实际无法触发。
static void
FuncKey_LongHold_Callback(Button* btn) {
    (void)btn;
    if (!force_shutdown_sent && millis() - func_key_pressed_ms >= FORCE_SHUTDOWN_HOLD_MS) {
        force_shutdown_sent = true;
        CORE_DEBUG_PRINTF("[FuncKey] Force shutdown");
        systemInfo.powerMonitor.Force_ShutDown = true;
    }
}

static void
FuncKey_Release_Callback(Button* btn) {
    (void)btn;
    force_shutdown_sent = false;
}

static uint8_t
read_button_gpio(uint8_t button_id) {
    switch (button_id) {
        case POWER_KEY_ID: return digitalRead(POWER_KEY_PIN);
        case FUNCTION_KEY_ID: return digitalRead(FUNCTION_KEY_PIN);
        default: return 0;
    }
}

void
HAL::Key_Init() {
    CORE_DEBUG_PRINTF("KEY: Init");
    pinMode(POWER_KEY_PIN, INPUT_PULLUP);
    pinMode(FUNCTION_KEY_PIN, INPUT_PULLUP);

    // 电源键按下为高电平，功能键按下为低电平。
    button_init(&PowerKey, read_button_gpio, 1, POWER_KEY_ID);
    button_attach(&PowerKey, BTN_LONG_PRESS_HOLD, PowerKey_Pressing_Callback);
    button_attach(&PowerKey, BTN_PRESS_UP, PowerKey_Release_Callback);

    button_init(&FuncKey, read_button_gpio, 0, FUNCTION_KEY_ID);
    button_attach(&FuncKey, BTN_DOUBLE_CLICK, FuncKey_DoubleClick_Callback);
    button_attach(&FuncKey, BTN_PRESS_DOWN, FuncKey_PressDown_Callback);
    button_attach(&FuncKey, BTN_LONG_PRESS_HOLD, FuncKey_LongHold_Callback);
    button_attach(&FuncKey, BTN_PRESS_UP, FuncKey_Release_Callback);

    button_start(&PowerKey);
    button_start(&FuncKey);
}

void
HAL::Key_Update() {
    button_ticks();
}
