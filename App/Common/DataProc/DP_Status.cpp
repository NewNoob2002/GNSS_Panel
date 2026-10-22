#include "SystemService.h"
#include "DataProc.h"
#include "Log.h"

namespace {
DataProc::StatusSnapshot latest;
int on_event(DataNode*, DataNode::EventParam_t* event) {
    if (!event || event->event != DataNode::EVENT_PULL)
        return DataNode::RES_UNSUPPORTED_REQUEST;
    if (!event->data_p)
        return DataNode::RES_PARAM_ERROR;
    if (event->size != sizeof(latest))
        return DataNode::RES_SIZE_MISMATCH;
    std::memcpy(event->data_p, &latest, sizeof(latest));
    return DataNode::RES_OK;
}
} // namespace
DATA_PROC_INIT_DEF(Status) {
    latest = {};
    std::strcpy(latest.position.data(), "DEMO");
    latest.battery_valid = true;
    latest.battery_percent = 85;
    latest.battery_voltage = 7.60f;
    latest.charging = false;
    account->setEventCallback(on_event);
    return account->isRegistered();
}
bool StatusService::update(const DataProc::StatusSnapshot& status) {
    auto* account = DataProc::Broker()->search("Status");
    if (!account || status.position.back() != '\0')
        return false;
    latest = status;
    // 同步分发期间保留本次快照，重入更新不改写当前事件。
    const auto snapshot = latest;
    const int result = account->publish(&snapshot, sizeof(snapshot));
    if (result != DataNode::RES_OK && result != DataNode::RES_UNKNOWN) {
        APP_LOG_W("Status", "subscriber rejected update: %d", result);
        return false;
    }
    return true;
}
