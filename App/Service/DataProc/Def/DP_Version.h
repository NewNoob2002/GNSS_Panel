#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Board.h"

namespace dp {
/** @brief Version 领域的类型化同步读取、订阅接口。 */
using Version = Reader<names::Version, hal::BoardState>;
}
