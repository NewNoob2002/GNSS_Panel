#pragma once
#include <cstdint>
#include "SystemBackend.h"

/** @brief 平台初始化检查与能力元数据；不注册为粗 Status 设备。 */
class StatusDevice {
  public:
    /** @brief 返回编译期产品能力，无副作用。 */
    system_backend::Capabilities capabilities();
    /** @brief 返回静态步骤名。 */
    const char* initialization_name(unsigned step);
    /** @brief 原初始化步骤的本地化标题。 */
    i18n::TextId initialization_title(unsigned step);
    /** @brief 原有界探测与等待，输出缓冲区必须非空。 */
    DataProc::InitializationStatus poll_initialization_step(unsigned, uint32_t, char*, size_t);
};
