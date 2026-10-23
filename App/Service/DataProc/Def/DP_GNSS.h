#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_GNSS.h"

namespace dp {
/** @brief GNSS 领域的类型化同步读取、订阅接口。 */
using GNSS = Reader<names::GNSS, hal::GNSSState>;
}
