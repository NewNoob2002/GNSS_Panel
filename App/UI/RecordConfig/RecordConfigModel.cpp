#include "RecordConfigModel.h"
#include "Log.h"

using namespace page;

bool RecordConfigModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::Recorder>() && load<dp::Version>();
}

bool RecordConfigModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("RecordConfigModel", "StatusBar notification rejected");
    return ok;
}

void RecordConfigModel::sync_initial(const hal::RecorderState& recorder, bool record_settings) {
    state_.recorder = recorder;
    state_.record_settings = record_settings;
    state_.type = recorder.type;
    state_.interval = recorder.interval;
    feedback_ = {};
    if (recorder.pending)
        feedback_.begin();
    state_.feedback = feedback_.text;
    changed();
}

void RecordConfigModel::cycle_type() {
    state_.type = (state_.type + 1) % hal::kRecordTypeCount;
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void RecordConfigModel::cycle_interval() {
    state_.interval = (state_.interval + 1) % hal::kRecordIntervalCount;
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void RecordConfigModel::clear_feedback() {
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void RecordConfigModel::poll_feedback() {
    const auto prev = feedback_.text;
    feedback_.poll(state_.recorder.pending);
    if (feedback_.text != prev) {
        state_.feedback = feedback_.text;
        changed();
    }
}

bool RecordConfigModel::toggle() {
    const bool start = !state_.recorder.recording;
    feedback_.begin();
    state_.feedback = feedback_.text;
    const bool accepted = start ? dp::Recorder::notify(*this, {hal::RecordAction::Start, state_.type, state_.interval}) == RES_OK
                                : dp::Recorder::notify(*this, {hal::RecordAction::Stop, 0, 0}) == RES_OK;
    feedback_.finish(accepted, state_.recorder.pending);
    state_.feedback = feedback_.text;
    changed();
    return accepted;
}

int RecordConfigModel::onEvent(EventParam_t* event) {
    int result = RES_UNSUPPORTED_REQUEST;
    dp::Recorder::State recorder;
    dp::Version::State version;
    if ((result = dp::Recorder::receive(event, recorder)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(recorder);
    } else if ((result = dp::Version::receive(event, version)) != RES_UNSUPPORTED_REQUEST) {
        if (result == RES_OK)
            apply(version);
    }
    return result;
}

void RecordConfigModel::apply(const dp::Recorder::State& value) {
    if (state_.recorder.valid == value.valid && state_.recorder.recording == value.recording
        && state_.recorder.pending == value.pending && state_.recorder.type == value.type
        && state_.recorder.interval == value.interval && state_.recorder.space_mb == value.space_mb)
        return;
    const bool pending_changed = (state_.recorder.pending != value.pending);
    state_.recorder = value;
    if (value.valid && (value.recording || value.pending || !state_.record_settings)) {
        state_.type = value.type;
        state_.interval = value.interval;
    }
    if (pending_changed || feedback_.text != i18n::TextId::Count) {
        feedback_.poll(value.pending);
        state_.feedback = feedback_.text;
    }
    changed();
}

void RecordConfigModel::apply(const dp::Version::State& value) {
    if (state_.record_settings == value.capabilities.record_settings)
        return;
    state_.record_settings = value.capabilities.record_settings;
    changed();
}
