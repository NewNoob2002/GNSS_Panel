#include "SystemInfosModel.h"
#include "Log.h"

using namespace page;

bool SystemInfosModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::Work>() && load<dp::GNSS>() && load<dp::WiFi>()
           && load<dp::Battery>() && load<dp::Recorder>() && load<dp::Version>() && load<dp::Init>();
}

bool SystemInfosModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("SystemInfosModel", "StatusBar notification rejected");
    return ok;
}

int SystemInfosModel::onEvent(EventParam_t* event) {
    if (!event)
        return RES_PARAM_ERROR;
    int result = RES_UNSUPPORTED_REQUEST;
    dp::Work::State work;
    dp::GNSS::State gnss;
    dp::WiFi::State wifi;
    dp::Battery::State battery;
    dp::Recorder::State recorder;
    dp::Version::State version;
    dp::Init::State init;
    if ((result = dp::Work::receive(event, work)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(work);
    } else if ((result = dp::GNSS::receive(event, gnss)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(gnss);
    } else if ((result = dp::WiFi::receive(event, wifi)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(wifi);
    } else if ((result = dp::Battery::receive(event, battery)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(battery);
    } else if ((result = dp::Recorder::receive(event, recorder)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(recorder);
    } else if ((result = dp::Version::receive(event, version)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(version);
    } else if ((result = dp::Init::receive(event, init)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(init);
    }
    return result;
}

void SystemInfosModel::apply(const dp::Work::State& value) {
    if (state_.work == value)
        return;
    state_.work = value;
    changed();
}

void SystemInfosModel::apply(const dp::GNSS::State& value) {
    if (state_.gnss == value)
        return;
    state_.gnss = value;
    changed();
}

void SystemInfosModel::apply(const dp::WiFi::State& value) {
    if (state_.wifi == value)
        return;
    state_.wifi = value;
    changed();
}

void SystemInfosModel::apply(const dp::Battery::State& value) {
    if (state_.battery == value)
        return;
    state_.battery = value;
    changed();
}

void SystemInfosModel::apply(const dp::Recorder::State& value) {
    if (state_.recorder == value)
        return;
    state_.recorder = value;
    changed();
}

void SystemInfosModel::apply(const dp::Version::State& value) {
    if (state_.version == value)
        return;
    state_.version = value;
    changed();
}

void SystemInfosModel::apply(const dp::Init::State& value) {
    if (state_.init == value)
        return;
    state_.init = value;
    changed();
}
