#include "WorkSettingsModel.h"
#include "Log.h"

using namespace page;

bool WorkSettingsModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::Work>();
}

bool WorkSettingsModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("WorkSettingsModel", "StatusBar notification rejected");
    return ok;
}

void WorkSettingsModel::sync_initial(const hal::WorkState& work) {
    state_.work = work;
    state_.target = work.ntrip == hal::NtripRole::Server && !work.radio_on ? hal::WorkTarget::Ntrip
                    : work.mode == hal::WorkMode::Base                    ? hal::WorkTarget::Base
                                                                          : hal::WorkTarget::Rover;
    if (work.protocol < hal::kRadioProtocolCount)
        state_.protocol = work.protocol;
    if (work.channel < hal::kRadioChannelCount)
        state_.channel = work.channel;
    feedback_ = {};
    if (work.pending)
        feedback_.begin();
    state_.feedback = feedback_.text;
    changed();
}

void WorkSettingsModel::cycle_target() {
    state_.target = state_.target == hal::WorkTarget::Rover  ? hal::WorkTarget::Base
                    : state_.target == hal::WorkTarget::Base ? hal::WorkTarget::Ntrip
                                                             : hal::WorkTarget::Rover;
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void WorkSettingsModel::cycle_protocol() {
    state_.protocol = (state_.protocol + 1) % hal::kRadioProtocolCount;
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void WorkSettingsModel::cycle_channel() {
    state_.channel = (state_.channel + 1) % hal::kRadioChannelCount;
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void WorkSettingsModel::clear_feedback() {
    feedback_ = {};
    state_.feedback = i18n::TextId::Count;
    changed();
}

void WorkSettingsModel::select_channel_feedback() {
    feedback_.text = i18n::TextId::SelectChannel;
    state_.feedback = feedback_.text;
    changed();
}

void WorkSettingsModel::poll_feedback() {
    const auto prev = feedback_.text;
    feedback_.poll(state_.work.pending);
    if (feedback_.text != prev) {
        state_.feedback = feedback_.text;
        changed();
    }
}

bool WorkSettingsModel::apply() {
    feedback_.begin();
    state_.feedback = feedback_.text;
    const bool accepted = dp::Work::notify(*this, {state_.target, state_.protocol, state_.channel}) == RES_OK;
    feedback_.finish(accepted, state_.work.pending);
    state_.feedback = feedback_.text;
    changed();
    return accepted;
}

int WorkSettingsModel::onEvent(EventParam_t* event) {
    dp::Work::State work;
    const int result = dp::Work::receive(event, work);
    if (result == RES_OK)
        apply(work);
    return result;
}

void WorkSettingsModel::apply(const dp::Work::State& value) {
    if (state_.work == value)
        return;
    const bool pending_changed = (state_.work.pending != value.pending);
    state_.work = value;
    if (pending_changed || feedback_.text != i18n::TextId::Count) {
        feedback_.poll(value.pending);
        state_.feedback = feedback_.text;
    }
    changed();
}
