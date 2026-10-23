#pragma once
#include "DataProc.h"
#include "DemoStatus.h"

namespace test_input {
/** @brief 测试修改单一 Demo HAL 模型，再推进服务时钟；不直接发布应用快照。 */
inline void
status(const page::StatusBarState& s) {
    auto& m = demo::model();
    m.battery = {s.battery_valid,
                 s.battery_percent,
                 static_cast<float>(s.battery_percent),
                 s.battery_voltage,
                 s.battery_temperature_valid,
                 s.battery_temperature,
                 s.charging ? hal::ChargeState::Normal : hal::ChargeState::NotCharging,
                 false};
    m.gnss.satellites_valid = s.satellites_valid;
    m.gnss.satellites_used = s.satellites;
    m.clock.time_valid = s.clock_valid;
    m.clock.hour = s.hour;
    m.clock.minute = s.minute;
    m.clock.second = s.second;
    const bool record_changed = m.recorder.reported_recording != s.recording;
    m.recorder.recording = m.recorder.reported_recording = s.recording;
    m.wifi.on = s.wifi;
    lv_tick_inc(record_changed ? 500 : 200);
    DataProc_RunLoop();
}

/** @brief 旧快照样例只用于测试输入；实际状态仍由细设备读取。 */
inline bool
status(const DataProc::StatusSnapshot& s) {
    if (s.position.back()) {
        return false;
    }
    page::StatusBarState value;
    value.satellites = s.satellites;
    value.satellites_valid = s.satellites_valid;
    value.battery_percent = s.battery_percent;
    value.battery_valid = s.battery_valid;
    value.battery_voltage = s.battery_voltage;
    value.battery_temperature_valid = s.battery_temperature_valid;
    value.battery_temperature = s.battery_temperature;
    value.charging = s.charging;
    value.hour = s.hour;
    value.minute = s.minute;
    value.second = s.second;
    value.clock_valid = s.clock_valid;
    value.recording = s.recording;
    value.wifi = s.wifi;
    status(value);
    return true;
}

inline bool
shutdown(hal::PowerCause cause) {
    auto& p = demo::model().power;
    if (p.standby) {
        return false;
    }
    p.powered_on = p.shutting_down = true;
    p.cause = cause;
    lv_tick_inc(20);
    DataProc_RunLoop();
    return true;
}

inline bool
standby() {
    demo::model().power = {false, false, true, hal::PowerCause::User};
    lv_tick_inc(20);
    DataProc_RunLoop();
    return true;
}
} // namespace test_input
