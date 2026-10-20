#include "StarMapModel.h"
#include <algorithm>
#include "Common/ModelUtils.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

using namespace page;

bool StarMapModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<Account>("StarMapModel", DataProc::Center(), 0, this);
    account_->SetEventCallback(on_event);
    if (!account_->IsRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("StarMapModel", "Account initialization failed");
        return false;
    }
    return true;
}
void StarMapModel::deinit() {
    account_.reset();
}
bool StarMapModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->Notify("StatusBar", &request, sizeof(request)) == Account::RES_OK;
    if (!ok)
        APP_LOG_E("StarMapModel", "StatusBar notification rejected");
    return ok;
}
int StarMapModel::on_event(Account* account, Account::EventParam_t* event) {
    auto* self = static_cast<StarMapModel*>(account->UserData);
    return model_utils::receive(event, self->status_, self->state_);
}
