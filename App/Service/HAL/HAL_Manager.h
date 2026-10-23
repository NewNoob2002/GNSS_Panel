#pragma once
#include "DeviceManager.h"

namespace hal {
/** @brief 返回平台持有的九个细设备；初始化只标记可用，不改变硬件状态。 */
DeviceManager& manager();
}
