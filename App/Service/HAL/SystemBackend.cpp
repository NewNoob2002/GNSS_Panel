#include "SystemBackend.h"
#include "Def/HAL_Init.h"
#include <cstdio>
namespace system_backend {
int PowerObject::onRead(void* buffer, size_t size) { return hal::output(buffer, size, power_report()); }
int PowerObject::onWrite(const void* buffer, size_t size) {
    if (size != sizeof(DataProc::PowerAction)) return RES_PARAM_ERROR;
    DataProc::PowerAction action;
    std::memcpy(&action, buffer, sizeof(action));
    if (action != DataProc::PowerAction::Off && action != DataProc::PowerAction::Reboot) return RES_PARAM_ERROR;
    return execute_power(action) ? RES_OK : RES_UNKNOWN;
}
int PowerObject::onIoctl(IO_Cmd_t cmd, void*) {
    if (hal::command(cmd) != hal::kRequestPowerOn) return RES_UNSUPPORT;
    return request_power_on() ? RES_OK : RES_UNKNOWN;
}
const char* initialization_name(unsigned step) {
    hal::InitInfo value;
    value.step = step;
    (void)hal::ioctl(devices().getDevice(hal::names::Init), hal::kInitializationInfo, value);
    return value.name;
}
i18n::TextId initialization_title(unsigned step) {
    hal::InitInfo value;
    value.step = step;
    if (hal::ioctl(devices().getDevice(hal::names::Init), hal::kInitializationInfo, value) != DeviceObject::RES_OK)
        return i18n::TextId::Initializing;
    switch (value.stage) {
    case hal::InitStage::Configuration: return i18n::TextId::LoadingConfiguration;
    case hal::InitStage::BatteryGauge: return i18n::TextId::CheckingBatteryGauge;
    case hal::InitStage::Charger: return i18n::TextId::CheckingCharger;
    case hal::InitStage::HostPower: return i18n::TextId::PoweringHostBoard;
    case hal::InitStage::HostLink: return i18n::TextId::CheckingHostLink;
    case hal::InitStage::Services: return i18n::TextId::PreparingServices;
    case hal::InitStage::GNSS: return i18n::TextId::StartingGnssService;
    case hal::InitStage::Network: return i18n::TextId::PreparingNetwork;
    case hal::InitStage::Finalization: return i18n::TextId::FinalizingStartup;
    }
    return i18n::TextId::Initializing;
}
DataProc::InitializationStatus poll_initialization_step(unsigned step, uint32_t elapsed, char* detail, size_t size) {
    if (!detail || !size) return DataProc::InitializationStatus::Failed;
    hal::InitPoll value;
    value.step = step; value.elapsed_ms = elapsed;
    if (hal::ioctl(devices().getDevice(hal::names::Init), hal::kPollInitialization, value) != DeviceObject::RES_OK)
        return DataProc::InitializationStatus::Failed;
    std::snprintf(detail, size, "%s", value.detail.data());
    return static_cast<DataProc::InitializationStatus>(value.status);
}
}
