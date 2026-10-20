#pragma once
#include <cstddef>
#include "DataProc/DataProc_Def.h"

/** Platform implementation of system operations; exactly one per build (RK simulator:
 * Status/SimulatedSystem.cpp). Called by SystemService on the LVGL thread; must be bounded and must not
 * block on I/O. The platform reports asynchronous results through SystemService (see SystemService.h).
 */
namespace system_backend {
struct Capabilities {
    bool simulated;        // Show DEMO markers; data and power actions are not real.
    bool reboot_available; // SystemDash offers Reboot.
    bool wifi_available;   // SystemSettings may toggle Wi-Fi.
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
/** Short name of initialization step 0..kInitializationStepCount-1, for logs. */
const char* initialization_name(unsigned step);
/** Localized SystemLoading headline of the step (what is actually being initialized or checked). */
i18n::TextId initialization_title(unsigned step);
/** Run one bounded initialization step; writes a NUL-terminated detail. False marks a failure that is
 * shown and logged but never stops startup.
 */
bool initialize_step(unsigned step, char* detail, std::size_t size);
} // namespace system_backend
