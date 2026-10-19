#include <Arduino.h>
#include "ButtonEvent/ButtonEvent.h"
#include "HAL.h"

static ButtonEvent FuncKey;
static volatile uint8_t ForceShutdown_count = 0;

static bool EncoderEnable = true;
static volatile int32_t EncoderDiff = 0;

static void
FuncKey_callback(ButtonEvent* btn, int event) {
    (void)btn;
    switch (event) {
        case ButtonEvent::EVENT_DOUBLE_CLICKED: EncoderDiff++; break;
        case ButtonEvent::EVENT_CLICKED: EncoderDiff--; break;
        case ButtonEvent::EVENT_RELEASED: ForceShutdown_count = 0; break;
        case ButtonEvent::EVENT_LONG_PRESSED_REPEAT:
            ForceShutdown_count++;
            /*
						if(ForceShutdown_count == 5){
							systemInfo.powerMonitor.ExternalPowerChange = 1;
						}
						*/
            if (ForceShutdown_count == 10) {
                ForceShutdown_count = 0;
                systemInfo.powerMonitor.Force_ShutDown = true;
            }
            break;
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
    FuncKey.EventMonitor(digitalRead(FUNCTION_KEY_PIN) == LOW);
}

int32_t
HAL::Encoder_GetDiff() {
    int32_t diff = EncoderDiff;
    EncoderDiff = 0;
    return diff;
}

bool
HAL::Encoder_GetIsPush() {
    if (!EncoderEnable) {
        return false;
    }

    return digitalRead(POWER_KEY_PIN) == HIGH;
}

void
HAL::Encoder_SetEnable(bool en) {
    EncoderEnable = en;
}