#pragma once
#include "DataProc/DataProc.h"

/** System state provider. Power and initialization operations are delegated to system_backend. */
namespace SystemService {
DataProc::SystemState snapshot();
bool power(DataProc::PowerAction action);
/** Platform -> App, application thread. Each publishes System state; the UI observes it. */
bool set_powered_on(bool on);                    // Power-on confirmed; leaves standby.
bool begin_shutdown(DataProc::PowerCause cause); // Platform-initiated, non-cancellable shutdown.
bool enter_standby();                            // Power-off finished while the UI keeps running.
bool set_wifi(bool on, bool pending);            // Applied Wi-Fi state; pending while a toggle is in flight.
bool valid_date(const DataProc::DateTime& date);
unsigned days_in_month(unsigned year, unsigned month);
} // namespace SystemService

namespace StatusService {
/** Feed an owned snapshot to Status and its subscribers, on the application thread. */
bool update(const DataProc::StatusSnapshot& status);
} // namespace StatusService

namespace DeviceService {
/** Platform -> App, application thread: publish device/business data (Device provider). */
bool update(const DataProc::DeviceSnapshot& device);
/** Latest published device data. */
DataProc::DeviceSnapshot snapshot();
} // namespace DeviceService
