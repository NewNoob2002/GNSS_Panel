#include "Charger.h"
#include "GpioPin.h"
#include <HAL.h>
#include "bq40z50.h"
#include <delay.h>
#include "mp2762a.h"

namespace {

constexpr GpioPin chargerEnable(CHARGER_ENABLE_PIN);
constexpr GpioPin chargerSwitch(CHARGER_SWITCH_PIN);
constexpr GpioPin chargerControl(CHARGER_CTRL_PIN);
constexpr GpioPin chargerFast(CHARGER_CTRL_FAST_PIN);
constexpr GpioPin chargerDetect(CHARGER_ADC_DETECT_PIN);
constexpr GpioPin usbSwitch(USB_SWITCH_PIN);

// V1.5 主板：MP2762A。
class Mp2762Charger final : public HAL::Charger {
  public:
    const char*
    name() const override {
        return "MP2762A";
    }

    void
    begin(BatteryInfo_t& battery) override {
        mp2762registerReset();
        mp2762setFastChargeVoltageMv(6600);
        mp2762setPrechargeCurrentMa(880);
        mp2762setFastChargeCurrentMa(1600);
        battery.chargeStatus = static_cast<Charger_Status_t>(mp2762getChargeStatus());
    }

    void
    update(BatteryInfo_t& battery) override {
        const uint8_t status = mp2762getChargeStatus();
        if (CM_VALUE_IN_RANGE(status, 0, 2)) {
            battery.chargeStatus = static_cast<Charger_Status_t>(status);
        }
    }

  protected:
    bool
    apply_enabled(bool enabled, BatteryInfo_t& battery) override {
        uint8_t config;
        if (!mp2762setChargerEnabled(enabled, &config)) {
            return false;
        }
        battery.mp2762_cfg0 = config;
        return true;
    }
};

// V1.3 主板：无充电芯片，GPIO 控制使能与电流；充电器连续 3 次检测到才开始充电。
class GpioCharger final : public HAL::Charger {
  public:
    const char*
    name() const override {
        return "GPIO";
    }

    void
    begin(BatteryInfo_t&) override {
        chargerEnable.setMode(GPIO_OUTPUT);
        chargerSwitch.setMode(GPIO_OUTPUT);
        chargerControl.setMode(GPIO_INPUT_PULLUP);
        chargerFast.setMode(GPIO_INPUT_PULLUP);
        chargerDetect.setMode(GPIO_INPUT_PULLUP);
        usbSwitch.setMode(GPIO_OUTPUT);
    }

    void
    enter_standby() override {
        usbSwitch.write(true);
    }

    void
    power_on() override {
        // 未接 Type-C 供电时切回 USB 通路。
        if (!chargerDetect.read()) {
            usbSwitch.write(false);
        }
    }

    void
    update(BatteryInfo_t& battery) override {
        if (battery.ChargerDetect) {
            if (battery.ChargerPlugCount < 5) {
                battery.ChargerPlugCount++;
            }
        } else {
            battery.ChargerPlugCount = 0;
        }
        if (battery.ChargerDisable) {
            return;
        }
        if (battery.ChargerPlugCount >= 3) {
            chargerEnable.write(true);
            chargerSwitch.write(true); // 3 A 档
            battery.chargeStatus = chargerEnable.read() ? fastCharge : normalCharge;
        } else {
            battery.chargeStatus = notCharge;
            chargerEnable.write(false);
        }
    }

  protected:
    bool
    apply_enabled(bool enabled, BatteryInfo_t& battery) override {
        if (enabled) {
            battery.ChargerDisable = 0; // update() 只在允许充电时驱动引脚
            update(battery);
        } else {
            chargerEnable.write(false);
        }
        return true;
    }
};

Mp2762Charger mp2762_charger;
GpioCharger gpio_charger;

} // namespace

void
HAL::Charger::set_enabled(bool enabled, BatteryInfo_t& battery) {
    if (enabled == !battery.ChargerDisable || !apply_enabled(enabled, battery)) {
        return;
    }
    battery.ChargerDisable = enabled ? 0 : 1;
    if (!enabled) {
        battery.chargeStatus = notCharge;
    }
}

HAL::Charger&
HAL::Charger_Get() {
    if (systemInfo.board.online_device.mp2762) {
        return mp2762_charger;
    }
    return gpio_charger;
}

void
HAL::Charger_Protect(BatteryInfo_t& battery) {
    Charger& charger = Charger_Get();
    if (!battery.sample_valid) {
        // 电量计从未识别时交由充电芯片自身保护充电：深度放电电池的电量计可能需要充电电压才能唤醒。
        // 电量计在线但样本失效或过期时，无法判断温度，禁止充电。
        if (systemInfo.board.online_device.bq40z50) {
            charger.set_enabled(false, battery);
        } else {
            battery.ChargerOverTempCount = 0;
            battery.isOverTemp = 0;
            charger.set_enabled(true, battery);
        }
        return;
    }
    // 连续 20 次（主循环电源任务 500 ms 一次，约 10 s）超过上限才判定过温，避免单次读数抖动。
    if (battery.Temp_f >= BATTERY_TERMINATE_TEMP) {
        if (battery.ChargerOverTempCount <= 50) {
            battery.ChargerOverTempCount++;
        }
        if (battery.ChargerOverTempCount >= 20) {
            battery.isOverTemp = 1;
        }
    } else {
        battery.ChargerOverTempCount = 0;
        battery.isOverTemp = 0;
    }
    charger.set_enabled(!battery.isOverTemp, battery);
}

void
HAL::Battery_Sample(BatteryInfo_t& battery) {
    battery.current_valid = false;
    uint16_t percent, remaining, full, voltage_mv, temp_dk;
    if (!bq40z50ReadRegister16Checked(BQ40Z50_RELATIVE_STATE_OF_CHARGE, &percent)
        || !bq40z50ReadRegister16Checked(BQ40Z50_REMAINING_CAPACITY, &remaining)
        || !bq40z50ReadRegister16Checked(BQ40Z50_FULL_CHARGE_CAPACITY, &full)
        || !bq40z50ReadRegister16Checked(BQ40Z50_VOLTAGE, &voltage_mv)
        || !bq40z50ReadRegister16Checked(BQ40Z50_TEMPERATURE, &temp_dk) || full == 0U || percent > 100U
        || voltage_mv == 0U || temp_dk == 0U) {
        battery.sample_valid = false;
        return;
    }
    const float percent_f = static_cast<float>(remaining) / full * 100.0f;
    const uint16_t voltage = voltage_mv / 1000U;
    const float voltage_f = voltage_mv / 1000.0f;
    const float temp_f = temp_dk / 10.0f - 273.15f;
    battery.sample_valid = true;
    battery.last_sample_ms = millis();
    // 电流只用于功率显示；单独读取失败时保留温度保护所需的有效样本。
    int16_t current_ma;
    battery.current_valid = bq40z50ReadAverageCurrentChecked(&current_ma);
    if (battery.current_valid) {
        battery.current_ma = current_ma;
    }

    battery.Actual_Percent = percent;
    const uint16_t processed = Power_AdjustPercent(percent);
    battery.Processed_Percent = processed;

    battery.Percent = CM_SET_VALUE_IN_RANGE(processed, 0, 100);
    battery.Voltage = CM_SET_VALUE_IN_RANGE(voltage, 6, 9);
    battery.Temp = CM_SET_VALUE_IN_RANGE(temp_dk, 1, 65535);

    battery.Percent_f = CM_SET_VALUE_IN_RANGE(percent_f, 0.0, 100.0);
    battery.Voltage_f = CM_SET_VALUE_IN_RANGE(voltage_f, 6.0, 8.4);
    battery.Temp_f = CM_SET_VALUE_IN_RANGE(temp_f, 0.0, 100.0);
}
