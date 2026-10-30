#include "BusinessDevice.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>

#include "SystemState.h"
#include "Log.h"
#include "lvgl.h"
#include "mcu_config.h"

namespace {

// 页面选项索引 -> 主控协议编码（与 WorkSettings / RecordConfig 表一致）。
constexpr uint8_t kRadioProtocolCode[] = {1, 2, 4, 5, 9, 10, 13, 16};
constexpr uint8_t kRecordTypeCode[] = {0x01, 0x02};
constexpr uint8_t kRecordIntervalCode[] = {0xff, 0x00, 0x01, 0x02, 0x04, 0x18};
static_assert(sizeof(kRadioProtocolCode) == hal::kRadioProtocolCount);
static_assert(sizeof(kRecordTypeCode) == hal::kRecordTypeCount);
static_assert(sizeof(kRecordIntervalCode) == hal::kRecordIntervalCount);
static_assert(ChannelMax + 1 == hal::kRadioChannelCount, "index 0 is CUSTOM, then Channel1..9");

/** @brief 返回 code 在表中的索引；未知编码按第 0 项显示。 */
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

/** @brief 复制可能未以 NUL 结尾的定长字段，并保证目标结尾 NUL。 */
template <std::size_t N>
void
copy_text(std::array<char, N>& dst, const void* src, std::size_t max) {
    const std::size_t length = std::min(strnlen(static_cast<const char*>(src), max), N - 1);
    std::memcpy(dst.data(), src, length);
    dst[length] = '\0';
}
} // namespace

hal::BatteryState
BusinessDevice::battery() {
    const auto& power = systemInfo.power;
    const auto& battery = power.batteryInfo;
    const auto charge = battery.chargeStatus == fastCharge     ? hal::ChargeState::Fast
                        : battery.chargeStatus == normalCharge ? hal::ChargeState::Normal
                                                               : hal::ChargeState::NotCharging;
    return {battery.sample_valid,
            battery.Percent,
            battery.Percent_f,
            battery.Voltage_f,
            battery.sample_valid,
            battery.Temp_f,
            charge,
            power.ExternalPower == 1,
            battery.current_ma,
            battery.sample_valid && battery.current_valid};
}

hal::GNSSState
BusinessDevice::gnss() {
    const auto& position = systemInfo.host.position;
    hal::GNSSState value;
    value.position_valid = position.coordinate_status != position_none;
    value.latitude = position.coordinate_lat;
    value.longitude = position.coordinate_lon;
    value.altitude = position.coordinate_alt;
    value.satellites_valid = true;
    value.satellites_used = position.satellite_number_used;
    value.satellites_tracked = position.satellite_number_track;
#if PRODUCT_PROTOCOL_SATELLITE
    const auto& stars = systemInfo.host.starMap;
    value.constellations_valid = stars.sequence != 0;
    value.constellations = {stars.numberGPS,  stars.numberBDS,  stars.numberGLONASS, stars.numberGALILEO,
                            stars.numberSBAS, stars.numberQZSS, stars.numberIRNSS};
#endif
    switch (position.coordinate_status) {
        case position_single: value.fix = hal::FixStatus::Single; break;
        case position_float: value.fix = hal::FixStatus::Float; break;
        case position_fix: value.fix = hal::FixStatus::Fixed; break;
        default: break;
    }
    return value;
}

hal::WorkState
BusinessDevice::work() {
    const auto work = State::Effective_Work();
    return {true,
            systemInfo.request.work_pending,
            (work.work_mode == base_mode || work.work_mode == autobase_mode) ? hal::WorkMode::Base
                                                                             : hal::WorkMode::Rover,
            work.radio_status == On_Off_Status_ON,
            code_index(kRadioProtocolCode, work.radio_protocol),
            work.radio_channel < hal::kRadioChannelCount ? work.radio_channel : 0U,
            work.NtripServer_status == On_Off_Status_ON                    ? hal::NtripRole::Server
            : systemInfo.host.ntrip.NtripClient_status == On_Off_Status_ON ? hal::NtripRole::Client
                                                                           : hal::NtripRole::None};
}

hal::RecorderState
BusinessDevice::recorder() {
    const auto record = State::Effective_Record();
    hal::RecorderState value;
    value.valid = true;
    value.pending = systemInfo.request.record_op || systemInfo.request.record_settings_pending;
    value.recording = record.record_status == On_Off_Status_ON;
    value.type = code_index(kRecordTypeCode, record.record_type);
    value.interval = code_index(kRecordIntervalCode, record.record_interval);
    copy_text(value.name, record.record_name, sizeof(record.record_name));
    value.space_mb = record.record_leftspace;
    value.reported_recording = systemInfo.host.record.record_status == On_Off_Status_ON;
    return value;
}

hal::WifiState
BusinessDevice::wifi() {
    const auto& wifi = systemInfo.host.wifi;
    hal::WifiState value;
    value.valid = true;
    value.available = PRODUCT_SHUTDOWN_WIFI_BUTTON != 0;
    value.on = wifi.wifi_status == On_Off_Status_ON;
    value.pending = wifi_requested_;
    value.station = wifi.wifi_mode != 0;
    copy_text(value.ssid, wifi.wifi_ssid, sizeof(wifi.wifi_ssid));
    std::memcpy(value.ip.data(), wifi.wifi_ip, sizeof(wifi.wifi_ip));
    return value;
}

hal::BoardState
BusinessDevice::board(std::uint32_t now) {
    hal::BoardState value;
    const auto& board = systemInfo.board;
    copy_text(value.firmware, board.software_version, sizeof(board.software_version));
    copy_text(value.hardware, board.hardware_version, sizeof(board.hardware_version));
    std::snprintf(value.build.data(), value.build.size(), "%s %s", SOFTWARE_BUILD_DATE, SOFTWARE_BUILD_TIME);
    value.uptime_s = now / 1000U;
    return value;
}

void
BusinessDevice::update(std::uint32_t now) {
    const auto& wifi = systemInfo.host.wifi;
    const auto target = systemInfo.request.wifi_on;
    if (wifi_requested_ && wifi.wifi_status == target) {
        wifi_requested_ = false;
    } else if (wifi_requested_ && now - wifi_request_ms_ >= kWifiTimeoutMs) {
        wifi_requested_ = false;
        APP_LOG_W("Wifi", "host did not apply wifi %s within %lu ms", target ? "on" : "off",
                  static_cast<unsigned long>(kWifiTimeoutMs));
    }
}

bool
BusinessDevice::execute(const hal::WorkCommand& command) {
    if ((command.target != hal::WorkTarget::Rover && command.target != hal::WorkTarget::Base
         && command.target != hal::WorkTarget::Ntrip)
        || command.protocol >= hal::kRadioProtocolCount || command.channel >= hal::kRadioChannelCount
        || (command.target != hal::WorkTarget::Ntrip && !command.channel)) {
        return false;
    }
    auto work = State::Effective_Work();
    if (command.target == hal::WorkTarget::Ntrip) {
        work.work_mode = autobase_mode;
        work.radio_status = On_Off_Status_OFF;
        work.gprs_status = On_Off_Status_ON;
        work.NtripServer_status = On_Off_Status_ON;
    } else {
        const bool base = command.target == hal::WorkTarget::Base;
        work.work_mode = base ? autobase_mode : rover_mode;
        work.radio_status = On_Off_Status_ON;
        work.radio_mode = base ? radio_mode_tx : radio_mode_rx;
        work.radio_protocol = kRadioProtocolCode[command.protocol];
        work.radio_channel = static_cast<uint8_t>(command.channel);
        work.gprs_status = On_Off_Status_OFF;
        work.NtripServer_status = On_Off_Status_OFF;
    }
    State::Request_Work(work);
    return true;
}

bool
BusinessDevice::execute(const hal::RecorderCommand& command) {
    if ((command.action != hal::RecordAction::Start && command.action != hal::RecordAction::Stop)
        || (command.action == hal::RecordAction::Start
            && (command.type >= hal::kRecordTypeCount || command.interval >= hal::kRecordIntervalCount))) {
        return false;
    }
    const auto current = State::Effective_Record();
    uint8_t type = current.record_type, interval = current.record_interval;
#if PRODUCT_RECORD_SETTINGS
    if (command.action == hal::RecordAction::Start) {
        type = kRecordTypeCode[command.type];
        interval = kRecordIntervalCode[command.interval];
    }
#endif
    State::Request_Record(command.action == hal::RecordAction::Start ? On_Off_Status_ON : On_Off_Status_OFF, type,
                          interval);
    return true;
}

bool
BusinessDevice::request_wifi(bool on) {
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
    State::Request_Wifi(on);
    wifi_requested_ = true;
    wifi_request_ms_ = lv_tick_get();
    return true;
#else
    (void)on;
    return false;
#endif
}
