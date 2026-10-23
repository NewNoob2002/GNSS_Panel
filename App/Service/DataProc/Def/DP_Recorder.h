#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Recorder.h"

namespace dp {
/** @brief Recorder 领域的类型化同步读取、订阅与命令接口。 */
using Recorder = Writer<names::Recorder, hal::RecorderState, hal::RecorderCommand>;
}
