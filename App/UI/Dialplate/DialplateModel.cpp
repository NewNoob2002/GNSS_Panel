#include "DialplateModel.h"

using namespace page;

bool DialplateModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::GNSS>() && load<dp::Work>() && load<dp::Recorder>()
           && load<dp::Version>();
}

bool DialplateModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    return notify("StatusBar", &request, sizeof(request)) == RES_OK;
}

int DialplateModel::onEvent(EventParam_t* event) {
    int result = RES_UNSUPPORTED_REQUEST;
    dp::GNSS::State gnss;
    dp::Work::State work;
    dp::Recorder::State recorder;
    dp::Version::State version;
    if ((result = dp::GNSS::receive(event, gnss)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(gnss);
    } else if ((result = dp::Work::receive(event, work)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(work);
    } else if ((result = dp::Recorder::receive(event, recorder)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(recorder);
    } else if ((result = dp::Version::receive(event, version)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(version);
    }
    return result;
}

void DialplateModel::apply(const dp::GNSS::State& value) {
    if (state_.satellites_valid == value.satellites_valid && state_.satellites_used == value.satellites_used
        && state_.satellites_tracked == value.satellites_tracked)
        return;
    state_.satellites_valid = value.satellites_valid;
    state_.satellites_used = value.satellites_used;
    state_.satellites_tracked = value.satellites_tracked;
    changed();
}

void DialplateModel::apply(const dp::Work::State& value) {
    if (state_.work_valid == value.valid && state_.work_mode == value.mode && state_.radio_on == value.radio_on)
        return;
    state_.work_valid = value.valid;
    state_.work_mode = value.mode;
    state_.radio_on = value.radio_on;
    changed();
}

void DialplateModel::apply(const dp::Recorder::State& value) {
    if (state_.record_valid == value.valid && state_.recording == value.recording)
        return;
    state_.record_valid = value.valid;
    state_.recording = value.recording;
    changed();
}

void DialplateModel::apply(const dp::Version::State& value) {
    if (state_.star_map == value.capabilities.star_map)
        return;
    state_.star_map = value.capabilities.star_map;
    changed();
}
