#include "TemplateModel.h"

using namespace page;

bool TemplateModel::init() {
    dp::Recorder::State recorder;
    if (!ready() || !dp::Recorder::subscribe(*this) || dp::Recorder::pull(*this, recorder) != RES_OK)
        return false;
    state_ = recorder.recording;
    return true;
}

int TemplateModel::onEvent(EventParam_t* event) {
    dp::Recorder::State recorder;
    const int result = dp::Recorder::receive(event, recorder);
    if (result == RES_OK && recorder.recording != state_) {
        state_ = recorder.recording;
        changed();
    }
    return result;
}

bool TemplateModel::toggle_recording() {
    const dp::Recorder::Command command{state_ ? hal::RecordAction::Stop : hal::RecordAction::Start};
    return dp::Recorder::notify(*this, command) == RES_OK;
}
