#pragma once

namespace hal {
/** @brief 充电状态，与平台充电策略的三种状态对应。 */
enum class ChargeState { NotCharging, Normal, Fast };
/** @brief 电池同步值快照；只在应用线程读取，温度单位 °C，电压单位 V。 */
struct BatteryState {
    bool valid = false;
    unsigned level_percent = 0; ///< 0..100，策略调整后的电量，旧状态栏使用。
    float gauge_percent = 0; ///< 0..100，电量计原始电量，旧业务页使用。
    float voltage = 0; ///< V。
    bool temperature_valid = false;
    float temperature = 0; ///< °C。
    ChargeState charge{};
    bool external_power = false;
    bool operator==(const BatteryState& v) const {
        return valid == v.valid && level_percent == v.level_percent && gauge_percent == v.gauge_percent
            && voltage == v.voltage && temperature_valid == v.temperature_valid && temperature == v.temperature
            && charge == v.charge && external_power == v.external_power;
    }
    bool operator!=(const BatteryState& v) const { return !(*this == v); }
};
} // namespace hal
