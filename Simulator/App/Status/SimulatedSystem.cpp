#include <cstdio>
#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

// RK3506 host/test-board backend: every operation is simulated; no power, clock or storage syscalls.
namespace {
struct DemoStep {
    const char* name;
    bool success;
    std::uint32_t duration_ms; // Simulated check time: real steps differ, so the preview does too.
    const char* detail;
};
// Placeholder outcomes; the network node demonstrates a non-blocking failure.
constexpr DemoStep steps[] = {
    {"configuration", true, 0, "Rover, TRIMTALK, 455.05 MHz defaults loaded"},
    {"services", true, 400, "status and logging services prepared"},
    {"gnss", true, 1200, "GNSS receiver placeholder prepared"},
    {"network", false, 700, "Wi-Fi backend reserved; continue offline"},
    {"finalization", true, 200, "working UI prepared"},
};
static_assert(sizeof(steps) / sizeof(steps[0]) == DataProc::kInitializationStepCount);

void copy(char* dest, std::size_t size, const char* text) {
    std::snprintf(dest, size, "%s", text);
}
/** Simulated device data; commands change it as a real device would report back. */
DataProc::DeviceSnapshot device = [] {
    DataProc::DeviceSnapshot d;
    d.position_valid = true;
    d.latitude = 31.230400;
    d.longitude = 121.473700;
    d.altitude = 12.5;
    d.satellites_valid = true;
    d.satellites_used = 9;
    d.satellites_tracked = 14;
    d.constellations_valid = true;
    d.constellations = {4, 3, 1, 2, 0, 1, 0};
    d.work_valid = true;
    d.work_mode = DataProc::WorkMode::Rover;
    d.radio_protocol = 0;
    d.radio_channel = 1;
    d.record_valid = true;
    d.record_interval = 1;
    copy(d.record_name.data(), d.record_name.size(), "DEMO-001");
    d.record_space_mb = 1228.8f;
    d.wifi_valid = true;
    d.wifi_station = true;
    copy(d.wifi_ssid.data(), d.wifi_ssid.size(), "RK3506-DEMO");
    d.wifi_ip = {192, 168, 4, 2};
    d.power_valid = true;
    d.battery_percent = 85.0f;
    d.battery_voltage = 7.60f;
    d.battery_temperature = 28.5f;
    d.charge = DataProc::ChargeState::Normal;
    copy(d.firmware.data(), d.firmware.size(), "1.0.0");
    copy(d.hardware.data(), d.hardware.size(), "J V1.0");
    copy(d.build.data(), d.build.size(), "DEMO-001");
    d.uptime_s = 2 * 3600 + 15 * 60 + 30;
    return d;
}();
} // namespace

system_backend::Capabilities system_backend::capabilities() {
    return {.simulated = true,
            .reboot_available = true,
            .wifi_available = false,
            .star_map = true,
            .record_settings = true};
}
bool system_backend::request_power_on() {
    return true; // The Startup hold is the whole simulated power-on.
}
bool system_backend::execute_power(DataProc::PowerAction action) {
    const bool off = action == DataProc::PowerAction::Off;
    APP_LOG_I("SystemService", "simulated power %s: NO host/device power operation", off ? "OFF" : "REBOOT");
    // Power stays on in the simulator, so Off ends in standby at once; Reboot resumes the working root.
    return !off || SystemService::enter_standby();
}
i18n::TextId system_backend::initialization_title(unsigned step) {
    constexpr i18n::TextId titles[] = {i18n::TextId::LoadingConfiguration, i18n::TextId::PreparingServices,
                                       i18n::TextId::StartingGnssService, i18n::TextId::PreparingNetwork,
                                       i18n::TextId::FinalizingStartup};
    static_assert(sizeof(titles) / sizeof(titles[0]) == DataProc::kInitializationStepCount);
    return step < DataProc::kInitializationStepCount ? titles[step] : i18n::TextId::Initializing;
}
bool system_backend::request_wifi(bool) {
    return false; // The simulator has no Wi-Fi backend (wifi_available is false).
}
DataProc::DeviceSnapshot system_backend::initial_device() {
    return device;
}
bool system_backend::execute_device(const DataProc::DeviceRequest& request) {
    switch (request.command) {
        case DataProc::DeviceCommand::ApplyWork:
            device.ntrip =
                request.target == DataProc::WorkTarget::Ntrip ? DataProc::NtripRole::Server : DataProc::NtripRole::None;
            device.work_mode =
                request.target == DataProc::WorkTarget::Rover ? DataProc::WorkMode::Rover : DataProc::WorkMode::Base;
            device.radio_on = request.target != DataProc::WorkTarget::Ntrip;
            if (device.radio_on) {
                device.radio_protocol = request.radio_protocol;
                device.radio_channel = request.radio_channel;
            }
            break;
        case DataProc::DeviceCommand::StartRecord:
            device.recording = true;
            device.record_type = request.record_type;
            device.record_interval = request.record_interval;
            break;
        case DataProc::DeviceCommand::StopRecord:
            device.recording = false;
            break;
    }
    APP_LOG_I("SystemService", "simulated device command %d: NO host/device operation",
              static_cast<int>(request.command));
    return DeviceService::update(device);
}
const char* system_backend::initialization_name(unsigned step) {
    return step < DataProc::kInitializationStepCount ? steps[step].name : "invalid";
}
DataProc::InitializationStatus system_backend::poll_initialization_step(unsigned step, std::uint32_t elapsed_ms,
                                                                        char* detail, std::size_t size) {
    if (step >= DataProc::kInitializationStepCount || !detail || !size)
        return DataProc::InitializationStatus::Failed;
    if (elapsed_ms < steps[step].duration_ms)
        return DataProc::InitializationStatus::Pending;
    std::snprintf(detail, size, "%s", steps[step].detail);
    return steps[step].success ? DataProc::InitializationStatus::Ok : DataProc::InitializationStatus::Failed;
}
