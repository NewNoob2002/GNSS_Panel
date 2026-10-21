#include "PowerDevice.h"

#include "Common/SystemService.h"
#include "HAL.h"
#include "mcu_config.h"

namespace {

DataProc::PowerCause
shutdown_cause() {
    const Power_Monitor_t& power = systemInfo.power;
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
PowerDevice::update() {
    const auto power = system_backend::read_power();
    if (power.powered_on && !powered_on_) {
        // Startup 请求已同步发布 powered_on；此处幂等补报，并覆盖软件复位自动开机。
        SystemService::set_powered_on(true);
        shutdown_reported_ = standby_reported_ = false;
    }
    powered_on_ = power.powered_on;

    const bool shutting_down = power.shutting_down;
    if (powered_on_ && shutting_down && !shutdown_reported_) {
        shutdown_reported_ = SystemService::begin_shutdown(power.cause);
    }
    if (power.standby && !standby_reported_) {
        standby_reported_ = SystemService::enter_standby();
    }
}

/** 仅采样电源状态；revision/确认仍只属于显示协议。 */
system_backend::PowerReport
PowerDevice::power_report() {
    const auto& p = systemInfo.power;
    return {p.panel_power_on,
            bool(p.ShutdownGoing || p.ShutdownReq || p.LinuxPowerOff || p.LowBatteryPowerOff || p.Force_ShutDown),
            bool(p.ShutdownComplete), shutdown_cause()};
}

/** 完整长按请求保持原 HAL 的开机判定。 */
bool
PowerDevice::request_power_on() {
    return HAL::Power_RequestOn();
}

/** 只提交关机请求，确认与延时断电仍由 HAL 处理。 */
bool
PowerDevice::execute_power(DataProc::PowerAction action) {
    if (action != DataProc::PowerAction::Off) {
        return false;
    }
    HAL::Power_Shutdown(false);
    return true;
}
