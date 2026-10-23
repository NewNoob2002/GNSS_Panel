#include "SaveConfigModel.h"
#include "Log.h"

using namespace page;

bool SaveConfigModel::init() {
    return ready() && subscribe("StatusBar") && load<dp::Power>();
}

bool SaveConfigModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("SaveConfigModel", "StatusBar notification rejected");
    return ok;
}

bool SaveConfigModel::finish_power() {
    if (!state_.power_token)
        return false;
    return dp::Power::notify(*this, {dp::PowerOperation::Execute, {}, state_.power_token}) == RES_OK;
}

bool SaveConfigModel::cancel_power() {
    return dp::Power::notify(*this, {dp::PowerOperation::Cancel, {}, state_.power_token}) == RES_OK;
}

int SaveConfigModel::onEvent(EventParam_t* event) {
    dp::Power::State power;
    const int result = dp::Power::receive(event, power);
    if (result == RES_OK)
        apply(power);
    return result;
}

void SaveConfigModel::apply(const dp::Power::State& value) {
    if (state_.shutting_down == value.platform.shutting_down
        && state_.power_cause == value.platform.cause
        && state_.power_action == value.action
        && state_.power_pending == value.pending
        && state_.power_token == value.token)
        return;
    state_.shutting_down = value.platform.shutting_down;
    state_.power_cause = value.platform.cause;
    state_.power_action = value.action;
    state_.power_pending = value.pending;
    state_.power_token = value.token;
    changed();
}
