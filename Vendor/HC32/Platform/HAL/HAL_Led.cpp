#include "BlinkLed.h"
#include <HAL.h>
#include "SystemState.h"
#include <delay.h>

static BlinkLed powerLed(POWER_LED_PIN);
static BlinkLed chargerLed(CHARGE_LED_PIN);
static BlinkLed functionKeyLed(FUNCTION_LED_PIN);

static void
Led_Update_Charge() {
    if (systemInfo.power.ShutdownReq) {
        return;
    }
    // 关机驻留态，以及不在开机前显示电量的产品，未开机时只保留充电指示。
    const bool idle =
        systemInfo.power.ShutdownComplete || (!PRODUCT_LED_BATTERY_BEFORE_POWER_ON && !systemInfo.power.panel_power_on);
    if (idle) {
        powerLed.set_period(0);
    }
    const uint32_t now = millis();
    chargerLed.update(now);
    powerLed.update(now);

    if (systemInfo.power.batteryInfo.chargeStatus != 0x00) {
        // CORE_DEBUG_PRINTF("CHARGE\n");
        powerLed.write(false);
        powerLed.set_period(0);
        bool isFullCharge = (systemInfo.power.batteryInfo.Percent >= 100);
        if (isFullCharge) {
            chargerLed.set_period(0);
            chargerLed.write(true);
        } else {
            chargerLed.set_period(systemInfo.power.batteryInfo.chargeStatus == fastCharge ? 300 : 1000);
        }
    } else {
        chargerLed.write(false);
        chargerLed.set_period(0);
        if (idle) {
            return;
        }
        if (systemInfo.power.batteryInfo.Percent >= 14) {
            powerLed.write(true);
        } else if ((systemInfo.power.batteryInfo.Percent < 14) && (systemInfo.power.batteryInfo.Percent > 9)) {
            powerLed.set_period(1000);
        } else if ((systemInfo.power.batteryInfo.Percent <= 9) && (systemInfo.power.batteryInfo.Percent > 0)) {
            powerLed.set_period(300);
        }
    }
}

static void
Led_Update_Function() {
    if (systemInfo.power.ShutdownGoing || systemInfo.power.ShutdownComplete || !systemInfo.host.online) {
        return;
    }
    if (State::Effective_Record().record_status) {
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
