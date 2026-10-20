#include "SystemSettingsModel.h"
#include "Common/ModelUtils.h"
#include "Utils/Log/Log.h"

using namespace page;

bool SystemSettingsModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<Account>("SystemSettingsModel", DataProc::Center(), 0, this);
    account_->SetEventCallback(on_event);
    if (!account_->IsRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("SystemSettingsModel", "Account initialization failed");
        return false;
    }
    return true;
}
void SystemSettingsModel::deinit() {
    account_.reset();
}
bool SystemSettingsModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->Notify("StatusBar", &request, sizeof(request)) == Account::RES_OK;
    if (!ok)
        APP_LOG_E("SystemSettingsModel", "StatusBar notification rejected");
    return ok;
}
int SystemSettingsModel::on_event(Account* account, Account::EventParam_t* event) {
    auto* self = static_cast<SystemSettingsModel*>(account->UserData);
    return model_utils::receive(event, self->status_, self->state_);
}
bool SystemSettingsModel::toggle_language() {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::SetLanguage;
    request.language = state_.language == i18n::Language::English ? i18n::Language::Russian : i18n::Language::English;
    return account_ && account_->Notify("System", &request, sizeof(request)) == Account::RES_OK;
}
