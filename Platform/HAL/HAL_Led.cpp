#include "Arduino.h"
#include "BlinkLed.h"
#include "HAL.h"

static BlinkLed powerLed(POWER_LED_PIN);
static BlinkLed chargerLed(CHARGE_LED_PIN);
static BlinkLed functionKeyLed(FUNCTION_LED_PIN);

static void
Led_Update_Charge() {
    if (systemInfo.powerMonitor.ShutdownReq) {
        return;
    }
    // 关机驻留态，以及不在开机前显示电量的产品，未开机时只保留充电指示。
    const bool idle = systemInfo.powerMonitor.ShutdownComplete
                      || (!PRODUCT_LED_BATTERY_BEFORE_POWER_ON && !systemInfo.powerMonitor.panel_power_on);
    if (idle) {
        powerLed.set_period(0);
    }
    const uint32_t now = millis();
    chargerLed.update(now);
    powerLed.update(now);

    if (systemInfo.powerMonitor.batteryInfo.chargeStatus != 0x00) {
        // CORE_DEBUG_PRINTF("CHARGE\n");
        powerLed.write(false);
        powerLed.set_period(0);
        bool isFullCharge = (systemInfo.powerMonitor.batteryInfo.Percent >= 100);
        if (isFullCharge) {
            chargerLed.set_period(0);
            chargerLed.write(true);
        } else {
            chargerLed.set_period(systemInfo.powerMonitor.batteryInfo.chargeStatus == fastCharge ? 300 : 1000);
        }
    } else {
        chargerLed.write(false);
        chargerLed.set_period(0);
        if (idle) {
            return;
        }
        if (systemInfo.powerMonitor.batteryInfo.Percent >= 14) {
            powerLed.write(true);
        } else if ((systemInfo.powerMonitor.batteryInfo.Percent < 14)
                   && (systemInfo.powerMonitor.batteryInfo.Percent > 9)) {
            powerLed.set_period(1000);
        } else if ((systemInfo.powerMonitor.batteryInfo.Percent <= 9)
                   && (systemInfo.powerMonitor.batteryInfo.Percent > 0)) {
            powerLed.set_period(300);
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
        functionKeyLed.set_period(500);
    } else {
        functionKeyLed.write(false);
        functionKeyLed.set_period(0);
    }
    functionKeyLed.update(millis());
}

void
HAL::Led_Update() {
    Led_Update_Charge();
    Led_Update_Function();
}
