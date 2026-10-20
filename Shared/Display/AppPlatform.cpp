#include "AppPlatform.h"

#include <cstdio>
#include <cstring>

#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "HAL.h"
#include "P4App.h"
#include "Utils/I18n/I18n.h"
#include "Utils/Log/Log.h"
#include "mcu_config.h"

#if !defined(LVGL_SIMULATOR)
#include <Arduino.h>
#endif

namespace {

constexpr uint32_t kStatusPeriodMs = 200;

P4App* app = nullptr;
uint32_t last_status_ms = 0;
bool status_sent = false;

// 平台电源状态的上一轮值，只在变化时通知共享应用。
bool powered_on = false;
bool shutdown_reported = false;
bool standby_reported = false;

uint32_t
now_ms() {
    return lv_tick_get();
}

void
log_sink(AppLogLevel level, const char* module, const char* message, void*) {
    static const char* const names[] = {"D", "I", "W", "E"};
#if defined(LVGL_SIMULATOR)
    std::printf("[%s][%s] %s\n", names[level], module, message);
#else
    // Release 构建中 CORE_DEBUG_PRINTF 为空，日志不占串口。
    CORE_DEBUG_PRINTF("[%s][%s] %s", names[level], module, message);
    (void)names;
    (void)level;
    (void)module;
    (void)message;
#endif
}

// 与旧状态栏一致：基站模式显示 BASE（蓝），否则按定位状态着色。
void
position_text(page::StatusBarState& state) {
    i18n::TextId id = i18n::TextId::PositionNone;
    lv_color_t color = lv_palette_main(LV_PALETTE_RED);
    if (systemInfo.work_mode == base_mode || systemInfo.work_mode == autobase_mode) {
        id = i18n::TextId::PositionBase;
        color = lv_palette_main(LV_PALETTE_BLUE);
    } else {
        switch (systemInfo.positionInfo.coordinate_status) {
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

void
publish_status(uint32_t now) {
    page::StatusBarState state;
    position_text(state);
    state.satellites = systemInfo.positionInfo.satellite_number_used;
    state.satellites_valid = true;
    // 主控未下发时间，沿用旧状态栏：显示开机运行时长。
    const uint32_t seconds = now / 1000U;
    state.hour = (seconds / 3600U) % 24U;
    state.minute = (seconds / 60U) % 60U;
    state.second = seconds % 60U;
    state.clock_valid = true;
    // 旧状态栏的 SD 图标从未按录制状态点亮（相关代码已注释），保持一致。
    state.recording = false;
    state.wifi = systemInfo.wifiInfo.wifi_status == On_Off_Status_ON;
    const BatteryInfo_t& battery = systemInfo.powerMonitor.batteryInfo;
    state.battery_valid = battery.sample_valid;
    state.battery_percent = battery.Percent;
    state.charging = battery.chargeStatus != notCharge;
    state.battery_voltage = battery.Voltage_f;
    state.battery_temperature_valid = battery.sample_valid;
    state.battery_temperature = battery.Temp_f;
    app->update_status(state);
}

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

// HAL 是电源状态的唯一判定者：开机（Power 键原始电平 2 s）、关机请求与断电后的待机驻留。
void
forward_power_events() {
    const Power_Monitor_t& power = systemInfo.powerMonitor;
    if (power.panel_power_on && !powered_on) {
        SystemService::set_powered_on(true);
        shutdown_reported = standby_reported = false;
    }
    powered_on = power.panel_power_on;

    const bool shutting_down = power.ShutdownGoing || power.ShutdownReq || power.LinuxPowerOff
                               || power.LowBatteryPowerOff || power.Force_ShutDown;
    if (powered_on && shutting_down && !shutdown_reported) {
        shutdown_reported = SystemService::begin_shutdown(shutdown_cause());
    }
    if (power.ShutdownComplete && !standby_reported) {
        standby_reported = SystemService::enter_standby();
    }
}

} // namespace

// ---- system_backend：HC32 实现 --------------------------------------------------------------

system_backend::Capabilities
system_backend::capabilities() {
    return {.simulated = false,
            .reboot_available = false, // HC32 产品不提供重启
            .wifi_available = PRODUCT_SHUTDOWN_WIFI_BUTTON != 0,
            .star_map = PRODUCT_PAGE_STAR_MAP != 0,
            .record_settings = PRODUCT_RECORD_SETTINGS != 0};
}

bool
system_backend::request_power_on() {
#if defined(LVGL_SIMULATOR)
    // PC 模拟器没有电源键电平，界面长按即开机。
    systemInfo.powerMonitor.panel_power_on = true;
    systemInfo.powerMonitor.pannel_power_on_time = lv_tick_get();
    return true;
#else
    // 固件由 HAL 按 Power 键原始电平判定 2 s 长按（Power_OnCheck / Power_PowerOffMonitor），
    // App_Update() 检测到 panel_power_on 后通知共享应用。
    return false;
#endif
}

bool
system_backend::execute_power(DataProc::PowerAction action) {
    if (action != DataProc::PowerAction::Off) {
        return false;
    }
    // 只发出关机请求；断电时序（等待主控确认、延时）由 HAL 电源策略完成，结束后上报待机。
    HAL::Power_Shutdown(false);
    return true;
}

namespace {
struct InitStep {
    const char* name;
    i18n::TextId title;
};

constexpr InitStep kInitSteps[] = {
    {"configuration", i18n::TextId::LoadingConfiguration},
    {"battery-gauge", i18n::TextId::CheckingBatteryGauge},
    {"charger", i18n::TextId::CheckingCharger},
    {"host-power", i18n::TextId::PoweringHostBoard},
    {"ui", i18n::TextId::FinalizingStartup},
};
static_assert(sizeof(kInitSteps) / sizeof(kInitSteps[0]) == DataProc::kInitializationStepCount);
} // namespace

const char*
system_backend::initialization_name(unsigned step) {
    return step < DataProc::kInitializationStepCount ? kInitSteps[step].name : "invalid";
}

i18n::TextId
system_backend::initialization_title(unsigned step) {
    return step < DataProc::kInitializationStepCount ? kInitSteps[step].title : i18n::TextId::Initializing;
}

bool
system_backend::initialize_step(unsigned step, char* detail, std::size_t size) {
    if (!detail || size == 0) {
        return false;
    }
    switch (step) {
        case 0:
            std::snprintf(detail, size, "%s FW %s HW %s", PRODUCT_NAME, systemInfo.software_version,
                          systemInfo.hardware_version);
            return systemInfo.software_version[0] != '\0';
        case 1:
            std::snprintf(detail, size, "BQ40Z50 %s", systemInfo.online_device.bq40z50 ? "online" : "not detected");
            return systemInfo.online_device.bq40z50;
        case 2:
            std::snprintf(detail, size, "MP2762 %s", systemInfo.online_device.mp2762 ? "online" : "not detected");
            return systemInfo.online_device.mp2762;
        case 3: {
            // 主控在开机时才上电、需数十秒启动，此处只确认供电已打开，不检查通信。
#if defined(LVGL_SIMULATOR)
            const bool powered = true;
#else
            const bool powered = digitalRead(POWER_CONTROL_PIN) == HIGH;
#endif
            std::snprintf(detail, size, "host board power %s", powered ? "on" : "off");
            return powered;
        }
        case 4: std::snprintf(detail, size, "working UI prepared"); return true;
        default: std::snprintf(detail, size, "invalid step %u", step); return false;
    }
}

#if !defined(LVGL_SIMULATOR)
// 单片机上无法 abort：记录原因后软件复位。Boot 保留 SWRF，复位后直接开机。
void
app_fatal(const char* module, const char* reason) {
    app_log_write(APP_LOG_ERROR, module ? module : "App", "fatal: %s; resetting", reason ? reason : "unknown");
    HAL_NVIC_SystemReset();
    while (true) {}
}
#endif

// ---- 入口 ------------------------------------------------------------------------------------

void
App_Init() {
    app_log_set_sink(log_sink, nullptr);
    app = new P4App;
    if (!app->init({.battery_warning = PRODUCT_PAGE_BATTERY_WARNING != 0})) {
        app_fatal("App", "shared LVGL application initialization failed");
    }
}

void
App_Update() {
    if (!app) {
        return;
    }
    const uint32_t now = now_ms();
#if !defined(LVGL_SIMULATOR)
    // 两键电平按 RK3506 语义处理（Function 单击下一项/双击返回，Power 单击选择/双击提交/长按开机）。
    app->on_key(P4App::Key::Power, HAL::Key_PowerPressed());
    app->on_key(P4App::Key::Function, HAL::Key_FunctionPressed());
    app->poll_keys(now);
#endif
    forward_power_events();
    if (!status_sent || now - last_status_ms >= kStatusPeriodMs) {
        status_sent = true;
        last_status_ms = now;
        publish_status(now);
    }
}

void
App_Deinit() {
    delete app;
    app = nullptr;
}

P4App*
App_Get() {
    return app;
}
