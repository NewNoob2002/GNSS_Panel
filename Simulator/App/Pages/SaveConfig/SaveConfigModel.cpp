#include "SaveConfigModel.h"
#include <cstring>
#include "Common/ModelUtils.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

using namespace page;

bool SaveConfigModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<Account>("SaveConfigModel", DataProc::Center(), 0, this);
    account_->SetEventCallback(on_event);
    if (!account_->IsRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("SaveConfigModel", "Account initialization failed");
        return false;
    }
    return true;
}
void SaveConfigModel::deinit() {
    change_cb_ = {};
    account_.reset();
}
bool SaveConfigModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->Notify("StatusBar", &request, sizeof(request)) == Account::RES_OK;
    if (!ok)
        APP_LOG_E("SaveConfigModel", "StatusBar notification rejected");
    return ok;
}
int SaveConfigModel::on_event(Account* account, Account::EventParam_t* event) {
    auto* self = static_cast<SaveConfigModel*>(account->UserData);
    const int result = model_utils::receive(event, self->status_, self->state_);
    if (result == Account::RES_OK && event->event == Account::EVENT_PUB_PUBLISH && event->tran
        && std::strcmp(event->tran->ID, "System") == 0 && self->change_cb_)
        self->change_cb_();
    return result;
}
bool SaveConfigModel::cancel_power() {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::CancelPower;
    request.token = state_.power_token;
    return account_ && account_->Notify("System", &request, sizeof(request)) == Account::RES_OK;
}
bool SaveConfigModel::finish_power() {
    if (!state_.power_token)
        return false;
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::ExecutePower;
    request.token = state_.power_token;
    return account_ && account_->Notify("System", &request, sizeof(request)) == Account::RES_OK;
}
