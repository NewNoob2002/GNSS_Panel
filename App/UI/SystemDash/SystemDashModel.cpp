#include "SystemDashModel.h"
#include "Log.h"

using namespace page;

bool SystemDashModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::Settings>() && load<dp::WiFi>()
           && load<dp::Version>() && load<dp::Power>() && load<dp::Clock>();
}

bool SystemDashModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("SystemDashModel", "StatusBar notification rejected");
    return ok;
}

bool SystemDashModel::toggle_language() {
    const auto next = state_.language == i18n::Language::English ? i18n::Language::Russian : i18n::Language::English;
    return dp::Settings::notify(*this, {next}) == RES_OK;
}

bool SystemDashModel::cycle_brightness() {
    const auto next = static_cast<uint8_t>(state_.brightness >= 100 ? 20 : (state_.brightness / 20 + 1) * 20);
    return dp::Settings::notify(*this, {state_.language, next}) == RES_OK;
}

bool SystemDashModel::set_wifi(bool on) {
    return dp::WiFi::notify(*this, {on}) == RES_OK;
}

bool SystemDashModel::prepare_power(hal::PowerCommand action) {
    return dp::Power::notify(*this, {dp::PowerOperation::Prepare, action}) == RES_OK;
}

bool SystemDashModel::cancel_power() {
    return dp::Power::notify(*this, {dp::PowerOperation::Cancel, {}, state_.power_token}) == RES_OK;
}

int SystemDashModel::onEvent(EventParam_t* event) {
    int result = RES_UNSUPPORTED_REQUEST;
    dp::Settings::State settings;
    dp::WiFi::State wifi;
    dp::Version::State version;
    dp::Power::State power;
    dp::Clock::State clock;
    if ((result = dp::Settings::receive(event, settings)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(settings);
    } else if ((result = dp::WiFi::receive(event, wifi)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(wifi);
    } else if ((result = dp::Version::receive(event, version)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(version);
    } else if ((result = dp::Power::receive(event, power)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(power);
    } else if ((result = dp::Clock::receive(event, clock)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(clock);
    }
    return result;
}

void SystemDashModel::apply(const dp::Settings::State& value) {
    if (state_.language == value.language && state_.brightness == value.brightness)
        return;
    state_.language = value.language;
    state_.brightness = value.brightness;
    changed();
}

void SystemDashModel::apply(const dp::WiFi::State& value) {
    if (state_.wifi_available == value.available && state_.wifi_on == value.on
        && state_.wifi_pending == value.pending)
        return;
    state_.wifi_available = value.available;
    state_.wifi_on = value.on;
    state_.wifi_pending = value.pending;
    changed();
}

void SystemDashModel::apply(const dp::Version::State& value) {
    state_.reboot_available = value.capabilities.reboot_available;
    state_.simulated = value.capabilities.simulated;
}

void SystemDashModel::apply(const dp::Power::State& value) {
    state_.power_token = value.token;
}

void SystemDashModel::apply(const dp::Clock::State& value) {
    state_.date = value.date;
}
