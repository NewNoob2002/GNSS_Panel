#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Battery.h"

namespace dp {
/** @brief Battery 领域的类型化同步读取、订阅接口。 */
using Battery = Reader<names::Battery, hal::BatteryState>;
}
