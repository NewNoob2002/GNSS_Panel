#pragma once
#include <cstddef>
#include <cstdint>
#include "DataProc/DataProc_Def.h"

/** Platform implementation of system operations; exactly one per build (RK simulator:
 * Status/SimulatedSystem.cpp). Called by SystemService on the LVGL thread; must be bounded and must not
 * block on I/O. The platform reports asynchronous results through SystemService (see SystemService.h).
 */
namespace system_backend {
struct Capabilities {
    bool simulated;        // Show DEMO markers; data and power actions are not real.
    bool reboot_available; // SystemDash offers Reboot.
    bool wifi_available;   // SystemDash may toggle Wi-Fi.
    bool star_map;         // StarMap page, reached from the Dialplate satellite icon.
    bool record_settings;  // RecordConfig type/interval can be changed (otherwise display only).
};
Capabilities capabilities();
/** Startup hold completed. Return true to confirm power-on now; a platform that decides power-on itself
 * returns false and later calls SystemService::set_powered_on(true).
 */
bool request_power_on();
/** Execute a confirmed user power action; false on failure. Completion of Off is reported with
 * SystemService::enter_standby(), possibly later or never (when power is really removed).
 */
bool execute_power(DataProc::PowerAction action);
/** Wi-Fi on/off requested on SystemDash (only when wifi_available). The platform reports the applied
 * state, and whether a change is still pending, with SystemService::set_wifi().
 */
bool request_wifi(bool on);
/** Device/business data before the platform's first DeviceService::update(). */
DataProc::DeviceSnapshot initial_device();
/** Execute a validated business request (indexes already range-checked); false on failure. The platform
 * publishes data with DeviceService::update(). For asynchronous requests, set work_pending/record_pending
 * before returning true and clear it only when the host has received the request. Receipt is not proof
 * that the physical operation has finished.
 */
bool execute_device(const DataProc::DeviceRequest& request);
/** Short name of initialization step 0..kInitializationStepCount-1, for logs. */
const char* initialization_name(unsigned step);
/** Localized SystemLoading headline of the step (what is actually being initialized or checked). */
i18n::TextId initialization_title(unsigned step);
/** Advance initialization step 0..kInitializationStepCount-1 without blocking. SystemLoading polls it on the UI
 * thread about every 50 ms; elapsed_ms counts from the step's first poll (0). Return Pending while the check is
 * still running, otherwise Ok/Failed with a NUL-terminated detail. Each step's duration is the real check time;
 * the platform bounds it with its own timeout (SystemLoading also stops waiting after kStepLimitMs). A failure is
 * shown and logged but never stops startup.
 */
DataProc::InitializationStatus poll_initialization_step(unsigned step, std::uint32_t elapsed_ms, char* detail,
                                                        std::size_t size);
} // namespace system_backend
