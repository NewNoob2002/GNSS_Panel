#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Work.h"

namespace dp {
/** @brief Work 领域的类型化同步读取、订阅与命令接口。 */
using Work = Writer<names::Work, hal::WorkState, hal::WorkCommand>;
}
