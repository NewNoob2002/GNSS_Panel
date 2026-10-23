#pragma once
#include "DeviceManager.h"
class PowerDevice;
class BusinessDevice;
class StatusDevice;

/** @brief 固件和 Board 模拟器共用的静态领域设备表；参数对象的地址在应用重建时保持稳定。 */
DeviceManager& domain_devices(PowerDevice&, BusinessDevice&, StatusDevice&);
