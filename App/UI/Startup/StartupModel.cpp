#include "StartupModel.h"
#include <algorithm>
#include "Log.h"

using namespace page;

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
    const bool charging = value.charge != hal::ChargeState::NotCharging;
    if (state_.battery_percent == value.level_percent && state_.battery_valid == value.valid
        && state_.battery_voltage == value.voltage && state_.charging == charging)
        return;
    state_.battery_percent = value.level_percent;
    state_.battery_valid = value.valid;
    state_.battery_voltage = value.voltage;
    state_.charging = charging;
    changed();
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
