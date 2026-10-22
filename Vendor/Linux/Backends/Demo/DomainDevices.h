#pragma once
#include "SystemBackend.h"

/** @brief Demo 与 SDL Demo 共用静态领域设备，旧模拟状态仍由原后端持有。 */
DeviceManager& demo_domain_devices(system_backend::PowerObject&, system_backend::BusinessObject&,
                                   system_backend::StatusObject&);
