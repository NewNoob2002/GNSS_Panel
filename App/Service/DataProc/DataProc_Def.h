#pragma once

#include <array>
#include <tuple>
#include <cstdint>
#include "I18n.h"
#include "HAL/Def/HAL_Power.h"

// Native in-process DataNode payloads, not a wire/flash format. Delivery is application-thread-only.
namespace DataProc {
enum class StatusBarStyle { Default = 0, Transparent };
struct StatusBarPresentation {
    bool visible = true;
    StatusBarStyle style{};
};
struct StatusSnapshot {
    std::array<char, 32> position{};
    unsigned satellites = 0, battery_percent = 0, hour = 0, minute = 0, second = 0;
    bool satellites_valid = false, battery_valid = false, clock_valid = false, recording = false, wifi = false;
    bool charging = false;
    float battery_voltage = 7.60f;
    bool battery_temperature_valid = false;
    float battery_temperature = 25.0f; // Celsius.
    std::uint32_t position_color_rgb = 0xf44336;
    bool operator==(const StatusSnapshot& v) const {
        return position == v.position && satellites == v.satellites && battery_percent == v.battery_percent
            && hour == v.hour && minute == v.minute && second == v.second && satellites_valid == v.satellites_valid
            && battery_valid == v.battery_valid && clock_valid == v.clock_valid && recording == v.recording
            && wifi == v.wifi && charging == v.charging && battery_voltage == v.battery_voltage
            && battery_temperature_valid == v.battery_temperature_valid && battery_temperature == v.battery_temperature
            && position_color_rgb == v.position_color_rgb;
    }
};
/** @brief 过渡 Device 聚合快照；选项下标与旧页面列表一致，协议码转换仅属于平台。 */
constexpr unsigned kRadioProtocolCount = 8, kRadioChannelCount = 10, kRecordTypeCount = 2, kRecordIntervalCount = 6;
constexpr std::size_t kConstellationCount = 7; // GPS, BDS, GLONASS, Galileo, SBAS, QZSS, IRNSS
enum class WorkMode { Rover, Base };
enum class NtripRole { None, Server, Client };
enum class ChargeState { NotCharging, Normal, Fast };
struct DeviceSnapshot {
    bool position_valid = false;
    double latitude = 0.0, longitude = 0.0, altitude = 0.0; // Degrees, degrees, metres.
    bool satellites_valid = false;
    unsigned satellites_used = 0, satellites_tracked = 0;
    bool constellations_valid = false;
    std::array<unsigned, kConstellationCount> constellations{};
    bool work_valid = false, work_pending = false; // pending until the host receives the request.
    WorkMode work_mode{};
    bool radio_on = false;
    unsigned radio_protocol = 0, radio_channel = 0; // Option indexes; channel 0 is CUSTOM.
    NtripRole ntrip{};
    bool record_valid = false, recording = false, record_pending = false;
    unsigned record_type = 0, record_interval = 0; // Option indexes.
    std::array<char, 17> record_name{};
    float record_space_mb = 0.0f;
    bool wifi_valid = false, wifi_station = false; // Station (STA) or access point (AP).
    std::array<char, 17> wifi_ssid{};
    std::array<std::uint8_t, 4> wifi_ip{};
    bool power_valid = false, external_power = false;
    float battery_percent = 0.0f, battery_voltage = 0.0f, battery_temperature = 0.0f;
    ChargeState charge{};
    std::array<char, 16> firmware{}, hardware{};
    std::array<char, 24> build{};
    std::uint32_t uptime_s = 0;
    bool operator==(const DeviceSnapshot& v) const {
        return std::tie(position_valid, latitude, longitude, altitude, satellites_valid, satellites_used,
            satellites_tracked, constellations_valid, constellations, work_valid, work_pending, work_mode,
            radio_on, radio_protocol, radio_channel, ntrip, record_valid, recording, record_pending, record_type,
            record_interval, record_name, record_space_mb, wifi_valid, wifi_station, wifi_ssid, wifi_ip,
            power_valid, external_power, battery_percent, battery_voltage, battery_temperature, charge,
            firmware, hardware, build, uptime_s)
            == std::tie(v.position_valid, v.latitude, v.longitude, v.altitude, v.satellites_valid, v.satellites_used,
            v.satellites_tracked, v.constellations_valid, v.constellations, v.work_valid, v.work_pending, v.work_mode,
            v.radio_on, v.radio_protocol, v.radio_channel, v.ntrip, v.record_valid, v.recording, v.record_pending,
            v.record_type, v.record_interval, v.record_name, v.record_space_mb, v.wifi_valid, v.wifi_station,
            v.wifi_ssid, v.wifi_ip, v.power_valid, v.external_power, v.battery_percent, v.battery_voltage,
            v.battery_temperature, v.charge, v.firmware, v.hardware, v.build, v.uptime_s);
    }
};
/** Work target chosen on WorkSettings: radio Rover/Base use protocol/channel; Ntrip is the network base. */
enum class WorkTarget { Rover, Base, Ntrip };
enum class DeviceCommand { ApplyWork, StartRecord, StopRecord };
struct DeviceRequest {
    DeviceCommand command{};
    WorkTarget target{};
    unsigned radio_protocol = 0, radio_channel = 0;
    unsigned record_type = 0, record_interval = 0;
};

struct DateTime {
    unsigned year = 2026, month = 9, day = 30, hour = 12, minute = 30;
    bool operator==(const DateTime& v) const {
        return std::tie(year, month, day, hour, minute) == std::tie(v.year, v.month, v.day, v.hour, v.minute);
    }
};
using PowerAction = hal::PowerCommand;
/** Why the system is shutting down. User is the SystemDash flow; the others come from the platform. */
using PowerCause = hal::PowerCause;
constexpr std::size_t kInitializationStepCount = 5;
// Pending: the step is still running (only returned by system_backend::poll_initialization_step, never stored).
enum class InitializationStatus { NotRun, Ok, Failed, Pending };
/** Last initialization attempt, retained by System until the next attempt/app teardown. UI thread only.
 * Detail is a NUL-terminated diagnostic message (maximum 95 bytes), not a serialized payload.
 */
struct InitializationResult {
    InitializationStatus status{};
    std::array<char, 96> detail{};
    bool operator==(const InitializationResult& v) const { return status == v.status && detail == v.detail; }
};
struct SystemState {
    i18n::Language language{};
    bool wifi_available = false, wifi_on = false, wifi_pending = false; // pending: toggle sent, not yet applied
    DateTime date{};
    bool simulated = true, power_pending = false;
    PowerAction power_action{};
    std::uint64_t power_token = 0, power_calls = 0;
    // Platform power state. powered_on: the platform confirmed power-on (Startup only requests it);
    // shutting_down: a platform-initiated, non-cancellable shutdown; standby: power-off has finished while
    // the UI keeps running (e.g. external power), so the UI returns to Startup.
    bool powered_on = false, shutting_down = false, standby = false, reboot_available = true;
    bool star_map = true, record_settings = true; // Product page capabilities (see system_backend).
    PowerCause power_cause{};
    bool initialization_started = false;
    // ponytail: retain only the latest attempt in RAM; add persistent history if earlier boots must be inspected.
    std::array<InitializationResult, kInitializationStepCount> initialization{};
    bool operator==(const SystemState& v) const {
        return std::tie(language, wifi_available, wifi_on, wifi_pending, date, simulated, power_pending,
            power_action, power_token, power_calls, powered_on, shutting_down, standby, reboot_available,
            star_map, record_settings, power_cause, initialization_started, initialization)
            == std::tie(v.language, v.wifi_available, v.wifi_on, v.wifi_pending, v.date, v.simulated, v.power_pending,
            v.power_action, v.power_token, v.power_calls, v.powered_on, v.shutting_down, v.standby, v.reboot_available,
            v.star_map, v.record_settings, v.power_cause, v.initialization_started, v.initialization);
    }
};
enum class SystemCommand {
    SetLanguage,
    SetTime,
    PreparePower,
    ExecutePower,
    CancelPower,
    BeginInitialization,
    RecordInitialization,
    RequestPowerOn,
    SetWifi
};
struct SystemRequest {
    SystemCommand command{};
    i18n::Language language{};
    DateTime date{};
    PowerAction action{};
    std::uint64_t token = 0;
    unsigned initialization_step = 0;
    bool wifi_on = false;
    InitializationResult initialization{};
};
} // namespace DataProc
