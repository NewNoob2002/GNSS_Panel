#include "StartupModel.h"
#include <algorithm>
#include "Log.h"
#include "FixedFormat.h"

using namespace page;

std::array<char, 24> page::startup_battery_reading(const StartupState& state) {
    std::array<char, 24> text{};
    if (!state.battery_valid) {
        lv_snprintf(text.data(), text.size(), "N/A");
    } else if (state.charging && state.battery_current_valid && state.battery_current_ma >= 50
               && state.battery_voltage > 0.0f && state.battery_voltage <= 65.535f) {
        // AverageCurrent 已由电量计平滑；先将 V 还原为 mV，随后用整数 mW 舍入到 0.1 W。
        // 有界比较同时排除 NaN/Inf；64 位乘法避免电压/电流边界值溢出。
        const auto voltage_mv = static_cast<uint32_t>(state.battery_voltage * 1000.0f + 0.5f);
        const uint32_t power_mw = static_cast<uint64_t>(voltage_mv) * state.battery_current_ma / 1000U;
        const uint32_t tenths = (power_mw + 50U) / 100U;
        lv_snprintf(text.data(), text.size(), "%lu.%luW", static_cast<unsigned long>(tenths / 10U),
                    static_cast<unsigned long>(tenths % 10U));
    } else {
        app::format_fixed(text.data(), text.size(), state.battery_voltage, 2, " V");
    }
    return text;
}

bool StartupModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::Battery>() && load<dp::Version>() && load<dp::Power>();
}

bool StartupModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("StartupModel", "StatusBar notification rejected");
    return ok;
}

bool StartupModel::request_power_on() {
    return dp::Power::notify(*this, {dp::PowerOperation::RequestOn}) == RES_OK;
}

int StartupModel::onEvent(EventParam_t* event) {
    int result = RES_UNSUPPORTED_REQUEST;
    dp::Battery::State battery;
    dp::Version::State version;
    dp::Power::State power;
    if ((result = dp::Battery::receive(event, battery)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(battery);
    } else if ((result = dp::Version::receive(event, version)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(version);
    } else if ((result = dp::Power::receive(event, power)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(power);
    }
    return result;
}

void StartupModel::apply(const dp::Battery::State& value) {
    const bool charging = value.valid && value.charge != hal::ChargeState::NotCharging;
    const unsigned percent = std::min(value.level_percent, 100U);
    const auto previous_reading = startup_battery_reading(state_);
    const bool display_changed = state_.battery_percent != percent || state_.battery_valid != value.valid
                                 || state_.charging != charging;
    state_.battery_percent = percent;
    state_.battery_valid = value.valid;
    state_.battery_voltage = value.voltage;
    state_.battery_current_ma = value.current_ma;
    state_.battery_current_valid = value.current_valid;
    state_.charging = charging;
    // 领域节点保留完整精度；页面只在最终文本/电量条发生变化时通知 View。
    if (display_changed || previous_reading != startup_battery_reading(state_)) {
        changed();
    }
}

void StartupModel::apply(const dp::Version::State& value) {
    if (state_.firmware == value.firmware && state_.hardware == value.hardware
        && state_.simulated == value.capabilities.simulated)
        return;
    state_.firmware = value.firmware;
    state_.hardware = value.hardware;
    state_.simulated = value.capabilities.simulated;
    changed();
}

void StartupModel::apply(const dp::Power::State& value) {
    // 非显示字段不通知：powered_on 仅供开机长按判定，不触发画面刷新。
    state_.powered_on = value.platform.powered_on;
}
