#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "DataProc.h"
#include "Utils/Log/Log.h"

namespace {
DataProc::DeviceSnapshot latest;
Account* provider = nullptr;

bool sender_is(const Account::EventParam_t* event, const char* name) {
    return event->tran && std::strcmp(event->tran->ID, name) == 0;
}
/** Only the page that owns an action may send it, and option indexes must be in range. */
bool valid(const Account::EventParam_t* event, const DataProc::DeviceRequest& request) {
    switch (request.command) {
        case DataProc::DeviceCommand::ApplyWork:
            return sender_is(event, "WorkSettingsModel")
                   && (request.target == DataProc::WorkTarget::Rover || request.target == DataProc::WorkTarget::Base
                       || request.target == DataProc::WorkTarget::Ntrip)
                   && request.radio_protocol < DataProc::kRadioProtocolCount
                   && request.radio_channel < DataProc::kRadioChannelCount;
        case DataProc::DeviceCommand::StartRecord:
            return sender_is(event, "RecordConfigModel") && request.record_type < DataProc::kRecordTypeCount
                   && request.record_interval < DataProc::kRecordIntervalCount;
        case DataProc::DeviceCommand::StopRecord:
            return sender_is(event, "RecordConfigModel");
        default:
            return false;
    }
}
int on_event(Account*, Account::EventParam_t* event) {
    if (!event || !event->data_p)
        return Account::RES_PARAM_ERROR;
    if (event->event == Account::EVENT_SUB_PULL) {
        if (event->size != sizeof(latest))
            return Account::RES_SIZE_MISMATCH;
        std::memcpy(event->data_p, &latest, sizeof(latest));
        return Account::RES_OK;
    }
    if (event->event != Account::EVENT_NOTIFY)
        return Account::RES_UNSUPPORTED_REQUEST;
    DataProc::DeviceRequest request;
    const int decoded = DataProc::ReadPayload(event, request);
    if (decoded != Account::RES_OK)
        return decoded;
    if (!valid(event, request))
        return Account::RES_PARAM_ERROR;
    return system_backend::execute_device(request) ? Account::RES_OK : Account::RES_UNKNOW;
}
} // namespace

DATA_PROC_INIT_DEF(Device) {
    provider = account;
    latest = system_backend::initial_device();
    account->SetEventCallback(on_event);
    return account->Commit(&latest, sizeof(latest));
}
bool DeviceService::update(const DataProc::DeviceSnapshot& device) {
    if (!provider || DataProc::Center()->SearchAccount("Device") != provider)
        return false;
    // Every text field must be NUL-terminated within its array before it is shown.
    if (device.record_name.back() || device.wifi_ssid.back() || device.firmware.back() || device.hardware.back()
        || device.build.back())
        return false;
    latest = device;
    if (!provider->Commit(&latest, sizeof(latest)))
        return false;
    const int result = provider->Publish();
    if (result != Account::RES_OK && result != Account::RES_UNKNOW) {
        APP_LOG_W("Device", "subscriber rejected update: %d", result);
        return false;
    }
    return true;
}
DataProc::DeviceSnapshot DeviceService::snapshot() {
    return latest;
}
