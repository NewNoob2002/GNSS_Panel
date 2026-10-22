#include "SystemBackend.h"
#include "SystemService.h"
#include "DataProc.h"
#include "Log.h"

namespace {
DataProc::DeviceSnapshot latest;
DataNode* provider = nullptr;

bool sender_is(const DataNode::EventParam_t* event, const char* name) {
    return event->tran && std::strcmp(event->tran->getID(), name) == 0;
}
/** Only the page that owns an action may send it, and option indexes must be in range. */
bool valid(const DataNode::EventParam_t* event, const DataProc::DeviceRequest& request) {
    switch (request.command) {
        case DataProc::DeviceCommand::ApplyWork:
            return sender_is(event, "WorkSettingsModel") && !latest.work_pending
                   && (request.target == DataProc::WorkTarget::Rover || request.target == DataProc::WorkTarget::Base
                       || request.target == DataProc::WorkTarget::Ntrip)
                   && request.radio_protocol < DataProc::kRadioProtocolCount
                   && request.radio_channel < DataProc::kRadioChannelCount
                   && (request.target == DataProc::WorkTarget::Ntrip || request.radio_channel != 0);
        case DataProc::DeviceCommand::StartRecord:
            return sender_is(event, "RecordConfigModel") && latest.record_valid && !latest.record_pending
                   && !latest.recording && request.record_type < DataProc::kRecordTypeCount
                   && request.record_interval < DataProc::kRecordIntervalCount;
        case DataProc::DeviceCommand::StopRecord:
            return sender_is(event, "RecordConfigModel") && latest.record_valid && !latest.record_pending
                   && latest.recording;
        default:
            return false;
    }
}
int on_event(DataNode*, DataNode::EventParam_t* event) {
    if (!event || !event->data_p)
        return DataNode::RES_PARAM_ERROR;
    if (event->event == DataNode::EVENT_PULL) {
        if (event->size != sizeof(latest))
            return DataNode::RES_SIZE_MISMATCH;
        std::memcpy(event->data_p, &latest, sizeof(latest));
        return DataNode::RES_OK;
    }
    if (event->event != DataNode::EVENT_NOTIFY)
        return DataNode::RES_UNSUPPORTED_REQUEST;
    DataProc::DeviceRequest request;
    const int decoded = DataProc::ReadPayload(event, request);
    if (decoded != DataNode::RES_OK)
        return decoded;
    if (!valid(event, request))
        return DataNode::RES_PARAM_ERROR;
    return system_backend::execute_device(request) ? DataNode::RES_OK : DataNode::RES_UNKNOWN;
}
} // namespace

DATA_PROC_INIT_DEF(Device) {
    provider = account;
    latest = system_backend::initial_device();
    account->setEventCallback(on_event);
    return account->isRegistered();
}
bool DeviceService::update(const DataProc::DeviceSnapshot& device) {
    if (!provider || DataProc::Broker()->search("Device") != provider)
        return false;
    // Every text field must be NUL-terminated within its array before it is shown.
    if (device.record_name.back() || device.wifi_ssid.back() || device.firmware.back() || device.hardware.back()
        || device.build.back())
        return false;
    latest = device;
    // 同步分发期间保留本次快照，重入更新不改写当前事件。
    const auto snapshot = latest;
    const int result = provider->publish(&snapshot, sizeof(snapshot));
    if (result != DataNode::RES_OK && result != DataNode::RES_UNKNOWN) {
        APP_LOG_W("Device", "subscriber rejected update: %d", result);
        return false;
    }
    return true;
}
DataProc::DeviceSnapshot DeviceService::snapshot() {
    return latest;
}
