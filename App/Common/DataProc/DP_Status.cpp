#include "Common/SystemService.h"
#include "DataProc.h"
#include "Utils/Log/Log.h"

namespace {
DataProc::StatusSnapshot latest;
int on_event(Account*, Account::EventParam_t* event) {
    if (!event || event->event != Account::EVENT_SUB_PULL)
        return Account::RES_UNSUPPORTED_REQUEST;
    if (!event->data_p)
        return Account::RES_PARAM_ERROR;
    if (event->size != sizeof(latest))
        return Account::RES_SIZE_MISMATCH;
    std::memcpy(event->data_p, &latest, sizeof(latest));
    return Account::RES_OK;
}
} // namespace
DATA_PROC_INIT_DEF(Status) {
    latest = {};
    std::strcpy(latest.position.data(), "DEMO");
    latest.battery_valid = true;
    latest.battery_percent = 85;
    latest.battery_voltage = 7.60f;
    latest.charging = false;
    account->SetEventCallback(on_event);
    return account->Commit(&latest, sizeof(latest));
}
bool StatusService::update(const DataProc::StatusSnapshot& status) {
    auto* account = DataProc::Center()->SearchAccount("Status");
    if (!account || status.position.back() != '\0')
        return false;
    latest = status;
    if (!account->Commit(&latest, sizeof(latest)))
        return false;
    const int result = account->Publish();
    if (result != Account::RES_OK && result != Account::RES_UNKNOW) {
        APP_LOG_W("Status", "subscriber rejected update: %d", result);
        return false;
    }
    return true;
}
