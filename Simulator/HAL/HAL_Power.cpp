#include "../../Platform/HAL/PowerPolicy.h"
#include "HAL.h"
#include "lvgl/lvgl.h"

static PowerPolicy power_policy;

void
HAL::Power_Init() {
    power_policy = PowerPolicy{};
}

void
HAL::Power_Shutdown(bool completed) {
    if (completed) {
        systemInfo.powerMonitor.ShutdownGoing = false;
        systemInfo.powerMonitor.ShutdownComplete = true;
    } else {
        systemInfo.powerMonitor.ShutdownReq = true;
    }
}

void
HAL::Power_PowerOffMonitor() {
    auto& power = systemInfo.powerMonitor;
    const bool requested = power.ShutdownReq || power.Force_ShutDown || power.LowBatteryPowerOff || power.LinuxPowerOff
                           || power.ShutdownEnsure;
    if (power_policy.state == PowerPolicy::Running && requested) {
        power.poweroff_flag = 1;
        power.ShutdownGoing = true;
        power.ShutdownReq = false;
    }
    if (power_policy.UpdateShutdown(lv_tick_get(), requested, power.ShutdownEnsure, power.Force_ShutDown)) {
        Power_Shutdown(true);
    }
}

bool
HAL::Power_ShutdownEnsure() {
    const bool value = systemInfo.powerMonitor.ShutdownEnsure;
    systemInfo.powerMonitor.ShutdownEnsure = false;
    return value;
}

bool
HAL::Power_ShutdownForce() {
    const bool value = systemInfo.powerMonitor.Force_ShutDown;
    systemInfo.powerMonitor.Force_ShutDown = false;
    return value;
}

bool
HAL::Power_ShutdownLowBattery() {
    const bool value = systemInfo.powerMonitor.LowBatteryPowerOff;
    systemInfo.powerMonitor.LowBatteryPowerOff = false;
    return value;
}

bool
HAL::Power_ShutdownSoftReset() {
    const bool value = systemInfo.powerMonitor.reset_flag;
    systemInfo.powerMonitor.reset_flag = false;
    return value;
}

void
HAL::Power_Update() {}

void
HAL::Power_GetInfo(Power_Monitor_t* info) {
    (void)info;
}

void
HAL::WatchDog_Feed() {}

const char*
HAL::Power_GetPowerOffCause() {
    if (systemInfo.powerMonitor.LinuxPowerOff) {
        return "Shutdown Board";
    }
    if (systemInfo.powerMonitor.LowBatteryPowerOff) {
        return "Shutdown LowBattery";
    }
    if (systemInfo.powerMonitor.Force_ShutDown) {
        return "Shutdown Force";
    }
    return "Shutdown PushKey";
}
