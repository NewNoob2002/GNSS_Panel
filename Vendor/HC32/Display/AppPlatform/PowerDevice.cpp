#include "PowerDevice.h"

#include <HAL.h>
#include "mcu_config.h"

namespace {

hal::PowerCause
shutdown_cause() {
    const Power_Monitor_t& power = systemInfo.power;
    if (power.LinuxPowerOff) {
        return hal::PowerCause::Host;
    }
    if (power.LowBatteryPowerOff) {
        return hal::PowerCause::LowBattery;
    }
    if (power.Force_ShutDown) {
        return hal::PowerCause::Forced;
    }
    return hal::PowerCause::User;
}

} // namespace

/** 仅采样电源状态；revision/确认仍只属于显示协议。 */
hal::PowerState
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
PowerDevice::execute_power(hal::PowerCommand action) {
    if (action != hal::PowerCommand::Off) {
        return false;
    }
    HAL::Power_Shutdown(false);
    return true;
}
