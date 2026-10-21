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
    if (!account_->IsRegistered() || !model_utils::subscribe(*account_, status_, state_, device_)) {
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
    const int result = model_utils::receive(event, self->status_, self->state_, self->device_);
    // Device/System changes only: the 5 Hz Status feed is shown by the StatusBar, not by this page.
    if (result == Account::RES_OK && event->event == Account::EVENT_PUB_PUBLISH && event->tran
        && std::strcmp(event->tran->ID, "Status") != 0 && self->change_cb_)
        self->change_cb_();
    return result;
}
