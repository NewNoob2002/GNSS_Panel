#include "RecordConfigModel.h"
#include <algorithm>
#include "ModelUtils.h"
#include "SystemService.h"
#include "Log.h"

using namespace page;

bool RecordConfigModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<DataNode>("RecordConfigModel", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !model_utils::subscribe(*account_, status_, state_, device_)) {
        account_.reset();
        APP_LOG_E("RecordConfigModel", "DataNode initialization failed");
        return false;
    }
    return true;
}
void RecordConfigModel::deinit() {
    account_.reset();
}
bool RecordConfigModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->notify("StatusBar", &request, sizeof(request)) == DataNode::RES_OK;
    if (!ok)
        APP_LOG_E("RecordConfigModel", "StatusBar notification rejected");
    return ok;
}
bool RecordConfigModel::start_record(unsigned record_type, unsigned record_interval) const {
    DataProc::DeviceRequest request{};
    request.command = DataProc::DeviceCommand::StartRecord;
    request.record_type = record_type;
    request.record_interval = record_interval;
    return model_utils::notify_device(account_, request);
}
bool RecordConfigModel::stop_record() const {
    DataProc::DeviceRequest request{};
    request.command = DataProc::DeviceCommand::StopRecord;
    return model_utils::notify_device(account_, request);
}
int RecordConfigModel::on_event(DataNode* account, DataNode::EventParam_t* event) {
    auto* self = static_cast<RecordConfigModel*>(account->getUserData());
    const int result = model_utils::receive(event, self->status_, self->state_, self->device_);
    // Device/System changes only: the 5 Hz Status feed is shown by the StatusBar, not by this page.
    if (result == DataNode::RES_OK && event->event == DataNode::EVENT_PUBLISH && event->tran
        && std::strcmp(event->tran->getID(), "Status") != 0 && self->change_cb_)
        self->change_cb_();
    return result;
}
