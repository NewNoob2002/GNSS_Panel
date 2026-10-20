#include "SystemState.h"

#include <cstring>

namespace {
SystemInfo_t storage;
} // namespace

const SystemInfo_t& systemInfo = storage;

BoardInfo_t&
State::Board() {
    return storage.board;
}

Power_Monitor_t&
State::Power() {
    return storage.power;
}

HostReport_t&
State::Host() {
    return storage.host;
}

Diagnostics_t&
State::Diagnostics() {
    return storage.diag;
}

void
State::Reset() {
    std::memset(&storage, 0, sizeof(storage));
}

void
State::Reset_HostData() {
    // revision 只增不减：复位前取的快照不能确认复位后的新请求。
    PanelRequest_t& request = storage.request;
    const uint32_t work = request.work_revision + 1U;
    const uint32_t record = request.record_revision + 1U;
    const uint32_t wifi = request.wifi_revision + 1U;
    std::memset(&storage.host, 0, sizeof(storage.host));
    std::memset(&request, 0, sizeof(request));
    request.work_revision = work;
    request.record_revision = record;
    request.wifi_revision = wifi;
}

WorkConfig_t
State::Effective_Work() {
    return storage.request.work_pending ? storage.request.work : storage.host.work;
}

RecordInfo_t
State::Effective_Record() {
    const PanelRequest_t& request = storage.request;
    RecordInfo_t record = storage.host.record;
    if (request.record_op || request.record_settings_pending) {
        record.record_status = request.record_status;
    }
    if (request.record_settings_pending) {
        record.record_type = request.record_type;
        record.record_interval = request.record_interval;
    }
    return record;
}

void
State::Request_Work(const WorkConfig_t& work) {
    storage.request.work = work;
    storage.request.work_pending = true;
    ++storage.request.work_revision;
}

void
State::Request_Record(On_Off_Status_t status, uint8_t type, uint8_t interval) {
    PanelRequest_t& request = storage.request;
    request.record_status = status;
    request.record_type = type;
    request.record_interval = interval;
    request.record_op = true;
    request.record_settings_pending = true;
    ++request.record_revision;
}

void
State::Request_RecordToggle() {
    const RecordInfo_t current = Effective_Record();
    PanelRequest_t& request = storage.request;
    request.record_status = current.record_status == On_Off_Status_ON ? On_Off_Status_OFF : On_Off_Status_ON;
    request.record_type = current.record_type;
    request.record_interval = current.record_interval;
    request.record_op = true;
    ++request.record_revision;
}

void
State::Request_RecordStop() {
    const RecordInfo_t current = Effective_Record();
    if (current.record_status == On_Off_Status_OFF) {
        return;
    }
    PanelRequest_t& request = storage.request;
    request.record_status = On_Off_Status_OFF;
    request.record_type = current.record_type;
    request.record_interval = current.record_interval;
    request.record_op = true;
    ++request.record_revision;
}

void
State::Request_Wifi(bool on) {
    storage.request.wifi_on = on ? On_Off_Status_ON : On_Off_Status_OFF;
    storage.request.wifi_pending = true;
    ++storage.request.wifi_revision;
}

void
State::Request_ForcedShutdown() {
    storage.power.Force_ShutDown = true;
}

uint32_t
State::Work_Revision() {
    return storage.request.work_revision;
}

void
State::Confirm_Work(uint32_t revision) {
    PanelRequest_t& request = storage.request;
    if (!request.work_pending || revision != request.work_revision) {
        return;
    }
    request.work_pending = false;
    storage.host.work = request.work;
}

uint32_t
State::Record_Revision() {
    return storage.request.record_revision;
}

void
State::Confirm_Record(uint32_t revision, bool settings) {
    PanelRequest_t& request = storage.request;
    if (revision != request.record_revision || !(request.record_op || (settings && request.record_settings_pending))) {
        return;
    }
    RecordInfo_t& host = storage.host.record;
    host.record_status = request.record_status;
    request.record_op = false;
    if (settings) {
        if (request.record_settings_pending) {
            host.record_type = request.record_type;
            host.record_interval = request.record_interval;
        }
        request.record_settings_pending = false;
    }
}

uint32_t
State::Wifi_Revision() {
    return storage.request.wifi_revision;
}

void
State::Confirm_Wifi(uint32_t revision) {
    if (revision == storage.request.wifi_revision) {
        storage.request.wifi_pending = false;
    }
}

// 旧协议在面板请求待确认时整帧忽略主控上报，因此只在没有待确认请求时回传。
void
State::Echo_HostWork() {
    if (!storage.request.work_pending) {
        Request_Work(storage.host.work);
    }
}

void
State::Echo_HostRecord() {
    PanelRequest_t& request = storage.request;
    if (request.record_settings_pending) {
        return;
    }
    const RecordInfo_t& host = storage.host.record;
    if (!request.record_op) { // 启停待确认时保留面板的启停请求
        request.record_status = host.record_status;
    }
    request.record_type = host.record_type;
    request.record_interval = host.record_interval;
    request.record_settings_pending = true;
    ++request.record_revision;
}

void
State::Host_ShutdownAcknowledged() {
    storage.power.poweroff_flag = 0;
    storage.power.ShutdownEnsure = true;
}

void
State::Host_ShutdownConfirmed() {
    Host_ShutdownAcknowledged();
    if (!storage.power.ShutdownGoing) {
        storage.power.LinuxPowerOff = true;
    }
}

void
State::Host_ShutdownRequested() {
    storage.power.poweroff_flag = 1;
    storage.power.LinuxPowerOff = true;
}

void
State::Host_ResetRequested(uint8_t reset) {
    storage.power.reset_flag = reset;
}

void
State::Host_ExternalPower(uint8_t external_power) {
    if (storage.power.ExternalPower && external_power == 0) {
        storage.power.ExternalPowerChange = 1;
    }
    storage.power.ExternalPower = external_power;
}
