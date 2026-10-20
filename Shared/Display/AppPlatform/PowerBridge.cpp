#include "PowerBridge.h"

#include "Common/SystemService.h"
#include "mcu_config.h"

namespace {

DataProc::PowerCause
shutdown_cause() {
    const Power_Monitor_t& power = systemInfo.powerMonitor;
    if (power.LinuxPowerOff) {
        return DataProc::PowerCause::Host;
    }
    if (power.LowBatteryPowerOff) {
        return DataProc::PowerCause::LowBattery;
    }
    if (power.Force_ShutDown) {
        return DataProc::PowerCause::Forced;
    }
    return DataProc::PowerCause::User;
}

} // namespace

void
PowerBridge::update() {
    const Power_Monitor_t& power = systemInfo.powerMonitor;
    if (power.panel_power_on && !powered_on_) {
        SystemService::set_powered_on(true);
        shutdown_reported_ = standby_reported_ = false;
    }
    powered_on_ = power.panel_power_on;

    const bool shutting_down = power.ShutdownGoing || power.ShutdownReq || power.LinuxPowerOff
                               || power.LowBatteryPowerOff || power.Force_ShutDown;
    if (powered_on_ && shutting_down && !shutdown_reported_) {
        shutdown_reported_ = SystemService::begin_shutdown(shutdown_cause());
    }
    if (power.ShutdownComplete && !standby_reported_) {
        standby_reported_ = SystemService::enter_standby();
    }
}
