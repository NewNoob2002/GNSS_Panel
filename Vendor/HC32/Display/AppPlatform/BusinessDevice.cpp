#include "BusinessDevice.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>

#include "SystemService.h"
#include "SystemState.h"
#include "Log.h"
#include "lvgl.h"
#include "mcu_config.h"

namespace {

// 页面选项索引 -> 主控协议编码（与旧 WorkSettings / RecordConfig 表一致）。
constexpr uint8_t kRadioProtocolCode[] = {1, 2, 4, 5, 9, 10, 13, 16};
constexpr uint8_t kRecordTypeCode[] = {0x01, 0x02};                             // XYZ, Rinex3.02
constexpr uint8_t kRecordIntervalCode[] = {0xff, 0x00, 0x01, 0x02, 0x04, 0x18}; // 不限, 15 min ... 24 h
static_assert(sizeof(kRadioProtocolCode) == DataProc::kRadioProtocolCount);
static_assert(sizeof(kRecordTypeCode) == DataProc::kRecordTypeCount);
static_assert(sizeof(kRecordIntervalCode) == DataProc::kRecordIntervalCount);
static_assert(ChannelMax + 1 == DataProc::kRadioChannelCount, "index 0 is CUSTOM, then Channel1..9");

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

} // namespace

DataProc::DeviceSnapshot
BusinessDevice::snapshot(std::uint32_t now) {
    DataProc::DeviceSnapshot device;
    // 填零后再赋值，便于按字节比较判断内容是否变化（含填充字节）。
    std::memset(static_cast<void*>(&device), 0, sizeof(device));

    const HostReport_t& host = systemInfo.host;
    const PositionInfo_t& position = host.position;
    device.position_valid = position.coordinate_status != position_none;
    device.latitude = position.coordinate_lat;
    device.longitude = position.coordinate_lon;
    device.altitude = position.coordinate_alt;
    device.satellites_valid = true;
    device.satellites_used = position.satellite_number_used;
    device.satellites_tracked = position.satellite_number_track;

#if PRODUCT_PROTOCOL_SATELLITE
    const StarMapInfo_t& stars = host.starMap;
    device.constellations_valid = stars.sequence != 0; // 收到过星图数据
    device.constellations = {stars.numberGPS,  stars.numberBDS,  stars.numberGLONASS, stars.numberGALILEO,
                             stars.numberSBAS, stars.numberQZSS, stars.numberIRNSS};
#endif

    const WorkConfig_t work = State::Effective_Work();
    device.work_valid = true;
    device.work_pending = systemInfo.request.work_pending;
    device.work_mode = (work.work_mode == base_mode || work.work_mode == autobase_mode) ? DataProc::WorkMode::Base
                                                                                        : DataProc::WorkMode::Rover;
    device.radio_on = work.radio_status == On_Off_Status_ON;
    device.radio_protocol = code_index(kRadioProtocolCode, work.radio_protocol);
    device.radio_channel = work.radio_channel < DataProc::kRadioChannelCount ? work.radio_channel : 0;
    device.ntrip = work.NtripServer_status == On_Off_Status_ON         ? DataProc::NtripRole::Server
                   : host.ntrip.NtripClient_status == On_Off_Status_ON ? DataProc::NtripRole::Client
                                                                       : DataProc::NtripRole::None;

    const RecordInfo_t record = State::Effective_Record();
    device.record_valid = true;
    device.record_pending = systemInfo.request.record_op || systemInfo.request.record_settings_pending;
    device.recording = record.record_status == On_Off_Status_ON;
    device.record_type = code_index(kRecordTypeCode, record.record_type);
    device.record_interval = code_index(kRecordIntervalCode, record.record_interval);
    copy_text(device.record_name, record.record_name, sizeof(record.record_name));
    device.record_space_mb = record.record_leftspace;

    const WifiInfo_t& wifi = host.wifi;
    device.wifi_valid = true;
    device.wifi_station = wifi.wifi_mode != 0;
    copy_text(device.wifi_ssid, wifi.wifi_ssid, sizeof(wifi.wifi_ssid));
    std::memcpy(device.wifi_ip.data(), wifi.wifi_ip, sizeof(wifi.wifi_ip));

    const Power_Monitor_t& power = systemInfo.power;
    const BatteryInfo_t& battery = power.batteryInfo;
    device.power_valid = battery.sample_valid;
    device.external_power = power.ExternalPower == 1;
    device.battery_percent = battery.Percent_f;
    device.battery_voltage = battery.Voltage_f;
    device.battery_temperature = battery.Temp_f;
    device.charge = battery.chargeStatus == fastCharge     ? DataProc::ChargeState::Fast
                    : battery.chargeStatus == normalCharge ? DataProc::ChargeState::Normal
                                                           : DataProc::ChargeState::NotCharging;

    const BoardInfo_t& board = systemInfo.board;
    copy_text(device.firmware, board.software_version, sizeof(board.software_version));
    copy_text(device.hardware, board.hardware_version, sizeof(board.hardware_version));
    std::snprintf(device.build.data(), device.build.size(), "%s %s", SOFTWARE_BUILD_DATE, SOFTWARE_BUILD_TIME);
    device.uptime_s = now / 1000U;
    return device;
}

void
BusinessDevice::update(std::uint32_t now) {
    forward_wifi(now);
    if (!polled_ || now - last_poll_ms_ >= kPeriodMs) {
        polled_ = true;
        last_poll_ms_ = now;
        publish(now);
    }
}

/** 内容变化时发布设备快照（运行时长每秒变化，即最多约每秒一次）。 */
void
BusinessDevice::publish(std::uint32_t now) {
    const DataProc::DeviceSnapshot device = system_backend::read_device().snapshot;
    (void)now;
    if (sent_ && std::memcmp(&device, &last_, sizeof(device)) == 0) {
        return;
    }
    if (DeviceService::update(device)) {
        sent_ = true;
        std::memcpy(static_cast<void*>(&last_), &device, sizeof(device));
    }
}

bool
BusinessDevice::execute(const DataProc::DeviceRequest& request, std::uint32_t now) {
    switch (request.command) {
        case DataProc::DeviceCommand::ApplyWork: {
            WorkConfig_t work = State::Effective_Work();
            if (request.target == DataProc::WorkTarget::Ntrip) {
                work.work_mode = autobase_mode;
                work.radio_status = On_Off_Status_OFF;
                work.gprs_status = On_Off_Status_ON;
                work.NtripServer_status = On_Off_Status_ON;
            } else {
                // 索引 0 为 CUSTOM，主控不接受；页面已拦截，此处兜底。
                if (request.radio_channel == 0) {
                    return false;
                }
                const bool base = request.target == DataProc::WorkTarget::Base;
                work.work_mode = base ? autobase_mode : rover_mode;
                work.radio_status = On_Off_Status_ON;
                work.radio_mode = base ? radio_mode_tx : radio_mode_rx;
                work.radio_protocol = kRadioProtocolCode[request.radio_protocol];
                work.radio_channel = static_cast<uint8_t>(request.radio_channel);
                work.gprs_status = On_Off_Status_OFF;
                work.NtripServer_status = On_Off_Status_OFF;
            }
            State::Request_Work(work);
            break;
        }
        case DataProc::DeviceCommand::StartRecord:
        case DataProc::DeviceCommand::StopRecord: {
            const RecordInfo_t current = State::Effective_Record();
            uint8_t type = current.record_type;
            uint8_t interval = current.record_interval;
#if PRODUCT_RECORD_SETTINGS
            if (request.command == DataProc::DeviceCommand::StartRecord) {
                type = kRecordTypeCode[request.record_type];
                interval = kRecordIntervalCode[request.record_interval];
            }
#endif
            const bool start = request.command == DataProc::DeviceCommand::StartRecord;
            State::Request_Record(start ? On_Off_Status_ON : On_Off_Status_OFF, type, interval);
            break;
        }
        default: return false;
    }
    publish(now);
    return true;
}

bool
BusinessDevice::request_wifi(bool on, std::uint32_t now) {
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
    // 设定值由面板协议发给主控，主控回报 wifi_status 后 forward_wifi() 结束等待。
    State::Request_Wifi(on);
    wifi_requested_ = true;
    wifi_request_ms_ = now;
    return true;
#else
    (void)on;
    (void)now;
    return false;
#endif
}

/** 上报主控的 Wi-Fi 状态（无开关按钮的产品也在 SystemInfos 显示）。
 * 回报与设定值一致前视为切换中；超时后放弃等待。 */
void
BusinessDevice::forward_wifi(std::uint32_t now) {
    const WifiInfo_t& wifi = systemInfo.host.wifi;
    const On_Off_Status_t target = systemInfo.request.wifi_on;
    if (wifi_requested_ && wifi.wifi_status == target) {
        wifi_requested_ = false;
    } else if (wifi_requested_ && now - wifi_request_ms_ >= kWifiTimeoutMs) {
        wifi_requested_ = false;
        APP_LOG_W("Wifi", "host did not apply wifi %s within %lu ms", target ? "on" : "off",
                  static_cast<unsigned long>(kWifiTimeoutMs));
    }
    SystemService::set_wifi(wifi.wifi_status == On_Off_Status_ON, wifi_requested_);
}

/** 同步返回快照和 Wi-Fi 状态；主控协议仍拥有请求确认。 */
system_backend::DeviceReport
BusinessDevice::device_report() {
    return {snapshot(lv_tick_get()), systemInfo.host.wifi.wifi_status == On_Off_Status_ON, wifi_requested_};
}

/** write 已按值解码和校验范围，此处保留原协议映射及发布。 */
bool
BusinessDevice::execute_device(const DataProc::DeviceRequest& request) {
    return execute(request, lv_tick_get());
}

/** ioctl 请求不等待主控回应。 */
bool
BusinessDevice::request_wifi(bool on) {
    return request_wifi(on, lv_tick_get());
}
