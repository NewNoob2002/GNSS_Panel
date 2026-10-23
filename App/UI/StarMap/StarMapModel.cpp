#include "StarMapModel.h"
#include "Log.h"

using namespace page;

bool StarMapModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::GNSS>() && load<dp::Version>();
}

bool StarMapModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("StarMapModel", "StatusBar notification rejected");
    return ok;
}

int StarMapModel::onEvent(EventParam_t* event) {
    int result = RES_UNSUPPORTED_REQUEST;
    dp::GNSS::State gnss;
    dp::Version::State version;
    if ((result = dp::GNSS::receive(event, gnss)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(gnss);
    } else if ((result = dp::Version::receive(event, version)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(version);
    }
    return result;
}

void StarMapModel::apply(const dp::GNSS::State& value) {
    if (state_.constellations_valid == value.constellations_valid
        && state_.constellations == value.constellations)
        return;
    state_.constellations_valid = value.constellations_valid;
    state_.constellations = value.constellations;
    changed();
}

void StarMapModel::apply(const dp::Version::State& value) {
    state_.simulated = value.capabilities.simulated;
}
