#pragma once

#include "DataProc/DataProc.h"

namespace model_utils {
/** Common transport validation only. The calling page Model owns its Account and state. */
inline bool subscribe(Account& account, DataProc::StatusSnapshot& status, DataProc::SystemState& system_state) {
    for (const auto* publisher : {"StatusBar", "Status", "System"})
        if (!account.Subscribe(publisher))
            return false;
    return account.Pull("Status", &status, sizeof(status)) == Account::RES_OK
           && account.Pull("System", &system_state, sizeof(system_state)) == Account::RES_OK;
}
inline int receive(Account::EventParam_t* event, DataProc::StatusSnapshot& status,
                   DataProc::SystemState& system_state) {
    if (!event || !event->tran || !event->data_p)
        return Account::RES_PARAM_ERROR;
    if (event->event == Account::EVENT_PUB_PUBLISH) {
        if (std::strcmp(event->tran->ID, "Status") == 0)
            return DataProc::ReadPayload(event, status);
        if (std::strcmp(event->tran->ID, "System") == 0)
            return DataProc::ReadPayload(event, system_state);
    } else if (event->event == Account::EVENT_SUB_PULL) {
        if (event->size == sizeof(status)) {
            std::memcpy(event->data_p, &status, sizeof(status));
            return Account::RES_OK;
        }
        if (event->size == sizeof(system_state)) {
            std::memcpy(event->data_p, &system_state, sizeof(system_state));
            return Account::RES_OK;
        }
        return Account::RES_SIZE_MISMATCH;
    }
    return Account::RES_UNSUPPORTED_REQUEST;
}
} // namespace model_utils
