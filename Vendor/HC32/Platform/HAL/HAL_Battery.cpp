#include "gpio.h"
#include "delay.h"
#include "Charger.h"
#include "HAL.h"
#include "SystemState.h"
#include "ots.h"

// 电池与充电监控：BQ40Z50 采样、充电控制、MCU 片内温度、看门狗。只在固件与主机测试中编译；
// 电源时序（含低电量关机）见 HAL_Power.cpp。

// 产品扩展点：默认无动作，X1 用于电量补偿。
__attribute__((weak)) void
HAL::Power_ProductUpdate() {}

__attribute__((weak)) uint16_t
HAL::Power_AdjustPercent(uint16_t actualPercent) {
    return actualPercent;
}

static void
ots_callback(void) {
    State::Power().batteryInfo.fOtsTemp = OTS_CalculateTemp();
}

void
HAL::Battery_Init() {
    OtsInitConfig(ots_callback);
}

void
HAL::Power_Update() {
    WatchDog_Feed();
    OtsStart();
    Power_ProductUpdate();
    Power_GetInfo(&State::Power());
}

void
HAL::Power_GetInfo(Power_Monitor_t* info) {
    if (info == nullptr) {
        return;
    }
    if (millis() - State::Power().BatteryLastHandleTime >= 3000) {
        State::Power().BatteryLastHandleTime = millis();
        if (!systemInfo.board.online_device.bq40z50) {
            digitalToggle(POWER_LED_PIN);
        } else {
            Battery_Sample(info->batteryInfo);
        }
    }
    if (info->batteryInfo.sample_valid && millis() - info->batteryInfo.last_sample_ms > 10000U) {
        info->batteryInfo.sample_valid = false;
    }
    info->batteryInfo.ChargerDetect = digitalRead(CHARGER_CTRL_PIN);
    Charger_Protect(info->batteryInfo);
    Charger_Get().update(info->batteryInfo);
}

void
HAL::WatchDog_Feed() {
    if (millis() - State::Power().WatchDogLastFeedTime >= WATCHDOG_FEED_TIME) {
        State::Power().WatchDogLastFeedTime = millis();
        digitalToggle(WATCHDOG_FEED_PIN);
    }
}
