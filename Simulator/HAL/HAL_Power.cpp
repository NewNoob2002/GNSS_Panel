#include "../../Platform/HAL/PowerPolicy.h"
#include "HAL.h"
#include "SystemState.h"
#include "lvgl/lvgl.h"

static PowerPolicy power_policy;

void
HAL::Power_Init() {
    power_policy = PowerPolicy{};
}

void
HAL::Power_Shutdown(bool completed) {
    if (completed) {
        State::Power().ShutdownGoing = false;
        State::Power().ShutdownComplete = true;
    } else {
        State::Power().ShutdownReq = true;
    }
}

void
HAL::Power_PowerOffMonitor() {
    auto& power = State::Power();
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
HAL::Power_ShutdownSoftReset() {
    const bool value = State::Power().reset_flag;
    State::Power().reset_flag = false;
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
    if (State::Power().LinuxPowerOff) {
        return "Shutdown Board";
    }
    if (State::Power().LowBatteryPowerOff) {
        return "Shutdown LowBattery";
    }
    if (State::Power().Force_ShutDown) {
        return "Shutdown Force";
    }
    return "Shutdown PushKey";
}
