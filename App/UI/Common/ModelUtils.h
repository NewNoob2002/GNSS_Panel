#pragma once

#include "DataProc/DataProc.h"

namespace model_utils {
/** Common transport validation only. The calling page Model owns its DataNode and state. */
inline bool subscribe(DataNode& account, DataProc::StatusSnapshot& status, DataProc::SystemState& system_state) {
    for (const auto* publisher : {"StatusBar", "Status", "System"})
        if (!account.subscribe(publisher))
            return false;
    return account.pull("Status", &status, sizeof(status)) == DataNode::RES_OK
           && account.pull("System", &system_state, sizeof(system_state)) == DataNode::RES_OK;
}
/** As above, plus the Device publisher for pages that show device/business data. */
inline bool subscribe(DataNode& account, DataProc::StatusSnapshot& status, DataProc::SystemState& system_state,
                      DataProc::DeviceSnapshot& device) {
    return subscribe(account, status, system_state) && account.subscribe("Device")
           && account.pull("Device", &device, sizeof(device)) == DataNode::RES_OK;
}
inline int receive(DataNode::EventParam_t* event, DataProc::StatusSnapshot& status,
                   DataProc::SystemState& system_state) {
    if (!event || !event->tran || !event->data_p)
        return DataNode::RES_PARAM_ERROR;
    if (event->event == DataNode::EVENT_PUBLISH) {
        if (std::strcmp(event->tran->getID(), "Status") == 0)
            return DataProc::ReadPayload(event, status);
        if (std::strcmp(event->tran->getID(), "System") == 0)
            return DataProc::ReadPayload(event, system_state);
    } else if (event->event == DataNode::EVENT_PULL) {
        if (event->size == sizeof(status)) {
            std::memcpy(event->data_p, &status, sizeof(status));
            return DataNode::RES_OK;
        }
        if (event->size == sizeof(system_state)) {
            std::memcpy(event->data_p, &system_state, sizeof(system_state));
            return DataNode::RES_OK;
        }
        return DataNode::RES_SIZE_MISMATCH;
    }
    return DataNode::RES_UNSUPPORTED_REQUEST;
}
inline int receive(DataNode::EventParam_t* event, DataProc::StatusSnapshot& status, DataProc::SystemState& system_state,
                   DataProc::DeviceSnapshot& device) {
    if (event && event->tran && event->event == DataNode::EVENT_PUBLISH
        && std::strcmp(event->tran->getID(), "Device") == 0)
        return DataProc::ReadPayload(event, device);
    return receive(event, status, system_state);
}
/** Send a business request to the Device provider (validated there). */
inline bool notify_device(const std::unique_ptr<DataNode>& account, const DataProc::DeviceRequest& request) {
    return account && account->notify("Device", &request, sizeof(request)) == DataNode::RES_OK;
}
} // namespace model_utils
