#include "Arduino.h"
#include "HAL.h"

static ledState_t powerLed = {0, 0, false};
static ledState_t chargerLed = {0, 0, false};
static ledState_t functionKeyLed = {0, 0, false};

static void
Led_Update_Charge() {
    if (systemInfo.powerMonitor.ShutdownReq) {
        return;
    }
    // 关机驻留态，以及不在开机前显示电量的产品，未开机时只保留充电指示。
    const bool idle = systemInfo.powerMonitor.ShutdownComplete
                      || (!PRODUCT_LED_BATTERY_BEFORE_POWER_ON && !systemInfo.powerMonitor.panel_power_on);
    if (idle) {
        powerLed.currentRate = 0;
    }
    const uint32_t now = millis();
    if (chargerLed.currentRate > 0) {
        if (now - chargerLed.lastToggleTime >= chargerLed.currentRate) {
            chargerLed.lastToggleTime = now;
            digitalToggle(CHARGE_LED_PIN);
        }
    }
    if (powerLed.currentRate > 0) {
        if (now - powerLed.lastToggleTime >= powerLed.currentRate) {
            powerLed.lastToggleTime = now;
            digitalToggle(POWER_LED_PIN);
        }
    }

    if (systemInfo.powerMonitor.batteryInfo.chargeStatus != 0x00) {
        // CORE_DEBUG_PRINTF("CHARGE\n");
        digitalWrite(POWER_LED_PIN, LOW);
        powerLed.currentRate = 0;
        bool isFullCharge = (systemInfo.powerMonitor.batteryInfo.Percent >= 100);
        if (isFullCharge) {
            chargerLed.currentRate = 0;
            digitalWrite(CHARGE_LED_PIN, HIGH);
        } else {
            if (systemInfo.powerMonitor.batteryInfo.chargeStatus == 0x02) {
                chargerLed.currentRate = 300;
            } else {
                chargerLed.currentRate = 1000;
            }
        }
    } else {
        digitalWrite(CHARGE_LED_PIN, LOW);
        chargerLed.currentRate = 0;
        if (idle) {
            return;
        }
        if (systemInfo.powerMonitor.batteryInfo.Percent >= 14) {
            digitalWrite(POWER_LED_PIN, HIGH);
        } else if ((systemInfo.powerMonitor.batteryInfo.Percent < 14)
                   && (systemInfo.powerMonitor.batteryInfo.Percent > 9)) {
            powerLed.currentRate = 1000;
        } else if ((systemInfo.powerMonitor.batteryInfo.Percent <= 9)
                   && (systemInfo.powerMonitor.batteryInfo.Percent > 0)) {
            powerLed.currentRate = 300;
        }
    }
}

static void
Led_Update_Function() {
    if (systemInfo.powerMonitor.ShutdownGoing || systemInfo.powerMonitor.ShutdownComplete
        || !systemInfo.online_device.eg25_board) {
        return;
    }
    if (systemInfo.recordInfo.record_status) {
        functionKeyLed.currentRate = 500;
    } else {
        digitalWrite(FUNCTION_LED_PIN, LOW);
        functionKeyLed.currentRate = 0;
    }
    /*Monitor*/
    if (functionKeyLed.currentRate > 0) {
        const uint32_t now = millis();
        if (now - functionKeyLed.lastToggleTime >= functionKeyLed.currentRate) {
            functionKeyLed.lastToggleTime = now;
            digitalToggle(FUNCTION_LED_PIN);
        }
    }
}

void
HAL::Led_Update() {
    Led_Update_Charge();
    Led_Update_Function();
}
