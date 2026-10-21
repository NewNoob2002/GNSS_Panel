#include "SystemInfosModel.h"
#include <algorithm>
#include "Common/ModelUtils.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

using namespace page;

bool SystemInfosModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<DataNode>("SystemInfosModel", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !model_utils::subscribe(*account_, status_, state_, device_)) {
        account_.reset();
        APP_LOG_E("SystemInfosModel", "DataNode initialization failed");
        return false;
    }
    return true;
}
void SystemInfosModel::deinit() {
    account_.reset();
}
bool SystemInfosModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->notify("StatusBar", &request, sizeof(request)) == DataNode::RES_OK;
    if (!ok)
        APP_LOG_E("SystemInfosModel", "StatusBar notification rejected");
    return ok;
}
int SystemInfosModel::on_event(DataNode* account, DataNode::EventParam_t* event) {
    auto* self = static_cast<SystemInfosModel*>(account->getUserData());
    const int result = model_utils::receive(event, self->status_, self->state_, self->device_);
    // Device/System changes only: the 5 Hz Status feed is shown by the StatusBar, not by this page.
    if (result == DataNode::RES_OK && event->event == DataNode::EVENT_PUBLISH && event->tran
        && std::strcmp(event->tran->getID(), "Status") != 0 && self->change_cb_)
        self->change_cb_();
    return result;
}
