#pragma once
#include <cstdint>
#include "Def/HAL_Init.h"
#include "Def/HAL_Board.h"

/** @brief 平台初始化检查与能力元数据，由 Init/Board 细设备使用。 */
class StatusDevice {
  public:
    /** @brief 返回编译期产品能力，无副作用。 */
    hal::Capabilities capabilities();
    /** @brief 返回静态步骤名。 */
    const char* initialization_name(unsigned step);
    /** @brief 原有界探测与等待，输出缓冲区必须非空。 */
    hal::InitStatus poll_initialization_step(unsigned, uint32_t, char*, size_t);
};
