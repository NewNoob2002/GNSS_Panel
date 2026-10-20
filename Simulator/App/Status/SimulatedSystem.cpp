#include <cstdio>
#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

// RK3506 host/test-board backend: every operation is simulated; no power, clock or storage syscalls.
namespace {
struct DemoStep {
    const char* name;
    bool success;
    const char* detail;
};
// Placeholder outcomes; the network node demonstrates a non-blocking failure.
constexpr DemoStep steps[] = {
    {"configuration", true, "Rover, TRIMTALK, 455.05 MHz defaults loaded"},
    {"services", true, "status and logging services prepared"},
    {"gnss", true, "GNSS receiver placeholder prepared"},
    {"network", false, "Wi-Fi backend reserved; continue offline"},
    {"finalization", true, "working UI prepared"},
};
static_assert(sizeof(steps) / sizeof(steps[0]) == DataProc::kInitializationStepCount);
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
const char* system_backend::initialization_name(unsigned step) {
    return step < DataProc::kInitializationStepCount ? steps[step].name : "invalid";
}
bool system_backend::initialize_step(unsigned step, char* detail, std::size_t size) {
    if (step >= DataProc::kInitializationStepCount || !detail || !size)
        return false;
    std::snprintf(detail, size, "%s", steps[step].detail);
    return steps[step].success;
}
