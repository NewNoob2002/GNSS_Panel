#include <Arduino.h>
#include "ButtonEvent/ButtonEvent.h"
#include "HAL.h"

// Function 键只在此处理长按强制关机；单击/双击等界面操作由共享应用按电平自行判定（AppPlatform）。
static ButtonEvent FuncKey;
static volatile uint8_t ForceShutdown_count = 0;

static void
FuncKey_callback(ButtonEvent* btn, int event) {
    (void)btn;
    switch (event) {
        case ButtonEvent::EVENT_RELEASED: ForceShutdown_count = 0; break;
        case ButtonEvent::EVENT_LONG_PRESSED_REPEAT:
            ForceShutdown_count++;
            if (ForceShutdown_count == 10) {
                ForceShutdown_count = 0;
                systemInfo.powerMonitor.Force_ShutDown = true;
            }
            break;
        default: break;
    }
}

void
HAL::Key_Init() {
    CORE_DEBUG_PRINTF("KEY: Init");
    pinMode(POWER_KEY_PIN, INPUT_PULLUP);
    pinMode(FUNCTION_KEY_PIN, INPUT_PULLUP);

    FuncKey.EventAttach(FuncKey_callback);
}

void
HAL::Key_Update() {
    FuncKey.EventMonitor(Key_FunctionPressed());
}

// Power 键按下为高电平（与 Power_PowerOffMonitor 开机判定一致），Function 键按下为低电平。
bool
HAL::Key_PowerPressed() {
    return digitalRead(POWER_KEY_PIN) == HIGH;
}

bool
HAL::Key_FunctionPressed() {
    return digitalRead(FUNCTION_KEY_PIN) == LOW;
}
