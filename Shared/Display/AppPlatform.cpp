#include "AppPlatform.h"

#include <algorithm>
#include <array>
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
constexpr uint32_t kDevicePeriodMs = 500;
// 旧界面切换 Wi-Fi 后无限等待主控回报；此处超时后不再显示 "..."（设定值仍保留，主控稍后仍可应用）。
constexpr uint32_t kWifiTimeoutMs = 15000;

// 页面选项索引 -> 主控协议编码（与旧 WorkSettings / RecordConfig 表一致）。
constexpr uint8_t kRadioProtocolCode[] = {1, 2, 4, 5, 9, 10, 13, 16};
constexpr uint8_t kRecordTypeCode[] = {0x01, 0x02};                             // XYZ, Rinex3.02
constexpr uint8_t kRecordIntervalCode[] = {0xff, 0x00, 0x01, 0x02, 0x04, 0x18}; // 不限, 15 min ... 24 h
static_assert(sizeof(kRadioProtocolCode) == DataProc::kRadioProtocolCount);
static_assert(sizeof(kRecordTypeCode) == DataProc::kRecordTypeCount);
static_assert(sizeof(kRecordIntervalCode) == DataProc::kRecordIntervalCount);
static_assert(ChannelMax + 1 == DataProc::kRadioChannelCount, "index 0 is CUSTOM, then Channel1..9");

P4App* app = nullptr;
uint32_t last_status_ms = 0;
bool status_sent = false;

// 平台电源状态的上一轮值，只在变化时通知共享应用。
bool powered_on = false;
bool shutdown_reported = false;
bool standby_reported = false;

uint32_t last_device_ms = 0;
bool device_polled = false;
bool device_sent = false; // last_device 有效
DataProc::DeviceSnapshot last_device;
// 星图数据曾经到达过（syncStatus 每收到一帧置位，读取后清零）。
bool constellations_seen = false;

// Wi-Fi 切换请求已发出、尚未被主控应用（仅 PRODUCT_SHUTDOWN_WIFI_BUTTON）。
bool wifi_requested = false;
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
uint32_t wifi_request_ms = 0;
#endif

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

/** 返回 code 在表中的索引；未知编码按第 0 项显示（与旧页面一致）。 */
template <std::size_t N>
unsigned
code_index(const uint8_t (&table)[N], uint8_t code) {
    for (std::size_t i = 0; i < N; ++i) {
        if (table[i] == code) {
            return static_cast<unsigned>(i);
        }
    }
    return 0;
}

/** 复制可能未以 NUL 结尾的定长字段（最多 max 字节），截断到 dst 容量并保证结尾 NUL。 */
template <std::size_t N>
void
copy_text(std::array<char, N>& dst, const void* src, std::size_t max) {
    const std::size_t length = std::min(strnlen(static_cast<const char*>(src), max), N - 1);
    std::memcpy(dst.data(), src, length);
    dst[length] = '\0';
}

DataProc::DeviceSnapshot
build_device(uint32_t now) {
    DataProc::DeviceSnapshot device;
    // 填零后再赋值，便于按字节比较判断内容是否变化（含填充字节）。
    std::memset(static_cast<void*>(&device), 0, sizeof(device));

    const PositionInfo_t& position = systemInfo.positionInfo;
    device.position_valid = position.coordinate_status != position_none;
    device.latitude = position.coordinate_lat;
    device.longitude = position.coordinate_lon;
    device.altitude = position.coordinate_alt;
    device.satellites_valid = true;
    device.satellites_used = position.satellite_number_used;
    device.satellites_tracked = position.satellite_number_track;

#if PRODUCT_PROTOCOL_SATELLITE
    StarMapInfo_t& stars = systemInfo.starMapInfo;
    if (stars.syncStatus) {
        constellations_seen = true;
        stars.syncStatus = false;
    }
    device.constellations_valid = constellations_seen;
    device.constellations = {stars.numberGPS,  stars.numberBDS,  stars.numberGLONASS, stars.numberGALILEO,
                             stars.numberSBAS, stars.numberQZSS, stars.numberIRNSS};
#endif

    const RadioInfo_t& radio = systemInfo.radioInfo;
    device.work_valid = true;
    device.work_mode = (systemInfo.work_mode == base_mode || systemInfo.work_mode == autobase_mode)
                           ? DataProc::WorkMode::Base
                           : DataProc::WorkMode::Rover;
    device.radio_on = radio.radio_status == On_Off_Status_ON;
    device.radio_protocol = code_index(kRadioProtocolCode, radio.radio_protocol);
    device.radio_channel = radio.radio_channel < DataProc::kRadioChannelCount ? radio.radio_channel : 0;
    const NtripInfo_t& ntrip = systemInfo.ntripInfo;
    device.ntrip = ntrip.NtripServer_status == On_Off_Status_ON   ? DataProc::NtripRole::Server
                   : ntrip.NtripClient_status == On_Off_Status_ON ? DataProc::NtripRole::Client
                                                                  : DataProc::NtripRole::None;

    const RecordInfo_t& record = systemInfo.recordInfo;
    device.record_valid = true;
    device.recording = record.record_status == On_Off_Status_ON;
    device.record_type = code_index(kRecordTypeCode, record.record_type);
    device.record_interval = code_index(kRecordIntervalCode, record.record_interval);
    copy_text(device.record_name, record.record_name, sizeof(record.record_name));
    device.record_space_mb = record.record_leftspace;

    const WifiInfo_t& wifi = systemInfo.wifiInfo;
    device.wifi_valid = true;
    device.wifi_station = wifi.wifi_mode != 0;
    copy_text(device.wifi_ssid, wifi.wifi_ssid, sizeof(wifi.wifi_ssid));
    std::memcpy(device.wifi_ip.data(), wifi.wifi_ip, sizeof(wifi.wifi_ip));

    const Power_Monitor_t& power = systemInfo.powerMonitor;
    const BatteryInfo_t& battery = power.batteryInfo;
    device.power_valid = battery.sample_valid;
    device.external_power = power.ExternalPower == 1;
    device.battery_percent = battery.Percent_f;
    device.battery_voltage = battery.Voltage_f;
    device.battery_temperature = battery.Temp_f;
    device.charge = battery.chargeStatus == fastCharge     ? DataProc::ChargeState::Fast
                    : battery.chargeStatus == normalCharge ? DataProc::ChargeState::Normal
                                                           : DataProc::ChargeState::NotCharging;

    copy_text(device.firmware, systemInfo.software_version, sizeof(systemInfo.software_version));
    copy_text(device.hardware, systemInfo.hardware_version, sizeof(systemInfo.hardware_version));
    std::snprintf(device.build.data(), device.build.size(), "%s %s", SOFTWARE_BUILD_DATE, SOFTWARE_BUILD_TIME);
    device.uptime_s = now / 1000U;
    return device;
}

/** 内容变化时发布设备快照（运行时长每秒变化，即最多约每秒一次）。 */
void
publish_device(uint32_t now) {
    const DataProc::DeviceSnapshot device = build_device(now);
    if (device_sent && std::memcmp(&device, &last_device, sizeof(device)) == 0) {
        return;
    }
    if (DeviceService::update(device)) {
        device_sent = true;
        std::memcpy(static_cast<void*>(&last_device), &device, sizeof(device));
    }
}

/** 上报主控的 Wi-Fi 状态（无开关按钮的产品也在 SystemInfos 显示）。
 * 回报与设定值一致前视为切换中；超时后放弃等待。 */
void
forward_wifi(uint32_t now) {
    const WifiInfo_t& wifi = systemInfo.wifiInfo;
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
    if (wifi_requested && wifi.wifi_status == wifi.wifi_on_off_set) {
        wifi_requested = false;
    } else if (wifi_requested && now - wifi_request_ms >= kWifiTimeoutMs) {
        wifi_requested = false;
        APP_LOG_W("Wifi", "host did not apply wifi %s within %lu ms", wifi.wifi_on_off_set ? "on" : "off",
                  static_cast<unsigned long>(kWifiTimeoutMs));
    }
#else
    (void)now;
#endif
    SystemService::set_wifi(wifi.wifi_status == On_Off_Status_ON, wifi_requested);
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

bool
system_backend::request_wifi(bool on) {
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
    // 设定值由面板协议发给主控（wifi_change_flag），主控回报 wifi_status 后 forward_wifi() 结束等待。
    systemInfo.wifiInfo.wifi_on_off_set = on ? On_Off_Status_ON : On_Off_Status_OFF;
    systemInfo.wifiInfo.wifi_change_flag = 1;
    wifi_requested = true;
    wifi_request_ms = now_ms();
    return true;
#else
    (void)on;
    return false;
#endif
}

DataProc::DeviceSnapshot
system_backend::initial_device() {
    return build_device(now_ms());
}

bool
system_backend::execute_device(const DataProc::DeviceRequest& request) {
    RadioInfo_t& radio = systemInfo.radioInfo;
    NtripInfo_t& ntrip = systemInfo.ntripInfo;
    RecordInfo_t& record = systemInfo.recordInfo;
    switch (request.command) {
        case DataProc::DeviceCommand::ApplyWork:
            if (request.target == DataProc::WorkTarget::Ntrip) {
                systemInfo.work_mode = autobase_mode;
                radio.radio_status = On_Off_Status_OFF;
                ntrip.gprs_status = On_Off_Status_ON;
                ntrip.NtripServer_status = On_Off_Status_ON;
            } else {
                // 索引 0 为 CUSTOM，主控不接受；页面已拦截，此处兜底。
                if (request.radio_channel == 0) {
                    return false;
                }
                const bool base = request.target == DataProc::WorkTarget::Base;
                systemInfo.work_mode = base ? autobase_mode : rover_mode;
                radio.radio_status = On_Off_Status_ON;
                radio.radio_mode = base ? radio_mode_tx : radio_mode_rx;
                radio.radio_protocol = kRadioProtocolCode[request.radio_protocol];
                radio.radio_channel = static_cast<uint8_t>(request.radio_channel);
                ntrip.gprs_status = On_Off_Status_OFF;
                ntrip.NtripServer_status = On_Off_Status_OFF;
            }
            radio.radio_change_flag = 1;
            break;
        case DataProc::DeviceCommand::StartRecord:
#if PRODUCT_RECORD_SETTINGS
            record.record_type = kRecordTypeCode[request.record_type];
            record.record_interval = kRecordIntervalCode[request.record_interval];
#endif
            record.record_status = On_Off_Status_ON;
            record.record_op = 1;
            record.record_change_flag = 1;
            break;
        case DataProc::DeviceCommand::StopRecord:
            record.record_status = On_Off_Status_OFF;
            record.record_op = 1;
            record.record_change_flag = 1;
            break;
        default: return false;
    }
    publish_device(now_ms());
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
            // V1.5 主板带 MP2762A；V1.3 没有该芯片，由 GPIO 控制充电，属正常配置。
            // 注意：V1.5 上 MP2762A 失效时与 V1.3 无法区分（版本本身由 I2C 扫描推断）。
            if (systemInfo.online_device.mp2762) {
                std::snprintf(detail, size, "MP2762A online (board %s)", systemInfo.hardware_version);
            } else {
                std::snprintf(detail, size, "GPIO charge control (board %s)", systemInfo.hardware_version);
            }
            return true;
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
    // 平台侧记录随应用实例重新开始（主机测试与模拟器会多次创建应用）。
    status_sent = device_polled = device_sent = false;
    powered_on = shutdown_reported = standby_reported = false;
    constellations_seen = wifi_requested = false;
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
    forward_wifi(now);
    if (!device_polled || now - last_device_ms >= kDevicePeriodMs) {
        device_polled = true;
        last_device_ms = now;
        publish_device(now);
    }
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
