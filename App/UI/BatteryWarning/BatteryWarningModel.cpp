#include "BatteryWarningModel.h"

using namespace page;

bool BatteryWarningModel::init() {
    dp::Battery::State battery;
    dp::Settings::State settings;
    if (!ready() || !dp::Battery::subscribe(*this) || !dp::Settings::subscribe(*this)
        || dp::Battery::pull(*this, battery) != RES_OK || dp::Settings::pull(*this, settings) != RES_OK)
        return false;
    state_ = {battery.temperature_valid, battery.temperature, settings.language};
    return true;
}

int BatteryWarningModel::onEvent(EventParam_t* event) {
    dp::Battery::State battery;
    int result = dp::Battery::receive(event, battery);
    if (result != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK && (state_.valid != battery.temperature_valid || state_.temperature != battery.temperature)) {
            state_.valid = battery.temperature_valid;
            state_.temperature = battery.temperature;
            changed();
        }
        return result;
    }
    dp::Settings::State settings;
    result = dp::Settings::receive(event, settings);
    if (result == RES_OK && state_.language != settings.language) {
        state_.language = settings.language;
        changed();
    }
    return result;
}
