#include "SystemDashModel.h"
#include <algorithm>
#include "Common/ModelUtils.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

using namespace page;

bool SystemDashModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<Account>("SystemDashModel", DataProc::Center(), 0, this);
    account_->SetEventCallback(on_event);
    if (!account_->IsRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("SystemDashModel", "Account initialization failed");
        return false;
    }
    return true;
}
void SystemDashModel::deinit() {
    account_.reset();
}
bool SystemDashModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->Notify("StatusBar", &request, sizeof(request)) == Account::RES_OK;
    if (!ok)
        APP_LOG_E("SystemDashModel", "StatusBar notification rejected");
    return ok;
}
int SystemDashModel::on_event(Account* account, Account::EventParam_t* event) {
    auto* self = static_cast<SystemDashModel*>(account->UserData);
    return model_utils::receive(event, self->status_, self->state_);
}
bool SystemDashModel::prepare_power(DataProc::PowerAction action) {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::PreparePower;
    request.action = action;
    return account_ && account_->Notify("System", &request, sizeof(request)) == Account::RES_OK;
}
bool SystemDashModel::cancel_power() {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::CancelPower;
    request.token = state_.power_token;
    return account_ && account_->Notify("System", &request, sizeof(request)) == Account::RES_OK;
}
