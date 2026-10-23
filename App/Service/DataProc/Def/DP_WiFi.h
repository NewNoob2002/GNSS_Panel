#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_WiFi.h"

namespace dp {
/** @brief WiFi 领域的类型化同步读取、订阅与命令接口。 */
using WiFi = Writer<names::WiFi, hal::WifiState, hal::WifiCommand>;
}
