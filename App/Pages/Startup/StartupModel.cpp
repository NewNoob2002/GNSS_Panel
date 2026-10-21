#include "StartupModel.h"
#include <algorithm>
#include "Common/ModelUtils.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

using namespace page;

bool StartupModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<DataNode>("StartupModel", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !model_utils::subscribe(*account_, status_, state_, device_)) {
        account_.reset();
        APP_LOG_E("StartupModel", "DataNode initialization failed");
        return false;
    }
    return true;
}
void StartupModel::deinit() {
    account_.reset();
}
bool StartupModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->notify("StatusBar", &request, sizeof(request)) == DataNode::RES_OK;
    if (!ok)
        APP_LOG_E("StartupModel", "StatusBar notification rejected");
    return ok;
}
bool StartupModel::request_power_on() const {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::RequestPowerOn;
    return account_ && account_->notify("System", &request, sizeof(request)) == DataNode::RES_OK;
}
int StartupModel::on_event(DataNode* account, DataNode::EventParam_t* event) {
    auto* self = static_cast<StartupModel*>(account->getUserData());
    const int res = model_utils::receive(event, self->status_, self->state_, self->device_);
    if (res == DataNode::RES_OK && event->event == DataNode::EVENT_PUBLISH && event->tran
        && (std::strcmp(event->tran->getID(), "Status") == 0 || std::strcmp(event->tran->getID(), "Device") == 0)
        && self->change_cb_)
        self->change_cb_();
    return res;
}
