#include "SystemDashModel.h"
#include <algorithm>
#include "ModelUtils.h"
#include "SystemService.h"
#include "Log.h"

using namespace page;

bool SystemDashModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<DataNode>("SystemDashModel", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("SystemDashModel", "DataNode initialization failed");
        return false;
    }
    return true;
}
void SystemDashModel::deinit() {
    change_cb_ = {};
    account_.reset();
}
bool SystemDashModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->notify("StatusBar", &request, sizeof(request)) == DataNode::RES_OK;
    if (!ok)
        APP_LOG_E("SystemDashModel", "StatusBar notification rejected");
    return ok;
}
int SystemDashModel::on_event(DataNode* account, DataNode::EventParam_t* event) {
    auto* self = static_cast<SystemDashModel*>(account->getUserData());
    const int result = model_utils::receive(event, self->status_, self->state_);
    if (result == DataNode::RES_OK && event->event == DataNode::EVENT_PUBLISH && event->tran
        && std::strcmp(event->tran->getID(), "System") == 0 && self->change_cb_)
        self->change_cb_();
    return result;
}
bool SystemDashModel::toggle_language() {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::SetLanguage;
    request.language = state_.language == i18n::Language::English ? i18n::Language::Russian : i18n::Language::English;
    return account_ && account_->notify("System", &request, sizeof(request)) == DataNode::RES_OK;
}
bool SystemDashModel::set_wifi(bool on) const {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::SetWifi;
    request.wifi_on = on;
    return account_ && account_->notify("System", &request, sizeof(request)) == DataNode::RES_OK;
}
bool SystemDashModel::prepare_power(DataProc::PowerAction action) {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::PreparePower;
    request.action = action;
    return account_ && account_->notify("System", &request, sizeof(request)) == DataNode::RES_OK;
}
bool SystemDashModel::cancel_power() {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::CancelPower;
    request.token = state_.power_token;
    return account_ && account_->notify("System", &request, sizeof(request)) == DataNode::RES_OK;
}
