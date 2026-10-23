#pragma once
#include <stdint.h>

// 电源时序独立于页面和动画；时间差使用无符号运算支持 tick 回绕。
struct PowerPolicy {
    enum State { Running, WaitingForHost, ShutdownDelay, Off } state = Running;

    uint32_t confirmed_ms = 0;

    uint32_t shutdown_started_ms = 0;
    uint32_t low_battery_started_ms = 0;
    bool low_battery_active = false;

    bool
    LowBatteryExpired(uint32_t now, bool low) {
        if (!low) {
            low_battery_active = false;
            return false;
        }
        if (!low_battery_active) {
            low_battery_active = true;
            low_battery_started_ms = now;
        }
        return now - low_battery_started_ms >= 180000U;
    }

    bool
    UpdateShutdown(uint32_t now, bool requested, bool acknowledged, bool forced) {
        if (state == Running && (requested || acknowledged)) {
            state = WaitingForHost;
            shutdown_started_ms = now;
        }
        if (state == WaitingForHost && acknowledged) {
            state = ShutdownDelay;
            confirmed_ms = now;
        }
        if ((state == WaitingForHost || state == ShutdownDelay)
            && (forced || now - shutdown_started_ms >= 20000U
                || (state == ShutdownDelay && now - confirmed_ms >= 8000U))) {
            state = Off;
            return true;
        }
        return false;
    }
};
