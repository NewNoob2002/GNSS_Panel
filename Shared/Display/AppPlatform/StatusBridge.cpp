#include "StatusBridge.h"

#include "App.h"
#include "SystemState.h"
#include "Utils/I18n/I18n.h"
#include "mcu_config.h"

namespace {

// 与旧状态栏一致：基站模式显示 BASE（蓝），否则按定位状态着色。
void
position_text(page::StatusBarState& state) {
    i18n::TextId id = i18n::TextId::PositionNone;
    lv_color_t color = lv_palette_main(LV_PALETTE_RED);
    const WorkMode_t mode = State::Effective_Work().work_mode;
    if (mode == base_mode || mode == autobase_mode) {
        id = i18n::TextId::PositionBase;
        color = lv_palette_main(LV_PALETTE_BLUE);
    } else {
        switch (systemInfo.host.position.coordinate_status) {
            case position_single:
                id = i18n::TextId::PositionSingle;
                color = lv_palette_main(LV_PALETTE_YELLOW);
                break;
            case position_fix:
                id = i18n::TextId::PositionFix;
                color = lv_palette_main(LV_PALETTE_GREEN);
                break;
            case position_float:
                id = i18n::TextId::PositionFloat;
                color = lv_palette_main(LV_PALETTE_YELLOW);
                break;
            default: break;
        }
    }
    state.position = i18n::text(id);
    state.position_color = color;
}

} // namespace

void
StatusBridge::update(App& app, std::uint32_t now) {
    if (sent_ && now - last_ms_ < kPeriodMs) {
        return;
    }
    sent_ = true;
    last_ms_ = now;

    page::StatusBarState state;
    position_text(state);
    state.satellites = systemInfo.host.position.satellite_number_used;
    state.satellites_valid = true;
    // 主控未下发时间，沿用旧状态栏：显示开机运行时长。
    const std::uint32_t seconds = now / 1000U;
    state.hour = (seconds / 3600U) % 24U;
    state.minute = (seconds / 60U) % 60U;
    state.second = seconds % 60U;
    state.clock_valid = true;
    // SD 图标跟随主控回报的实际录制状态；面板发出启停请求后，主控确认前不改变（等待提示在记录页）。
    state.recording = systemInfo.host.record.record_status == On_Off_Status_ON;
    state.wifi = systemInfo.host.wifi.wifi_status == On_Off_Status_ON;
    const BatteryInfo_t& battery = systemInfo.power.batteryInfo;
    state.battery_valid = battery.sample_valid;
    state.battery_percent = battery.Percent;
    state.charging = battery.chargeStatus != notCharge;
    state.battery_voltage = battery.Voltage_f;
    state.battery_temperature_valid = battery.sample_valid;
    state.battery_temperature = battery.Temp_f;
    app.update_status(state);
}
