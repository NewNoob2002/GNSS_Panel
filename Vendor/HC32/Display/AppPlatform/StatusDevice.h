#pragma once
#include <cstdint>
#include "SystemBackend.h"

/** systemInfo -> 状态栏快照，固定周期发布（含首次）。 */
class StatusDevice : public system_backend::StatusObject {
  public:
    /** 周期发布从设备 read 获取的状态栏快照。 */
    void update(std::uint32_t now);
    /** 状态栏原始数据到共享值快照的转换。 */
    DataProc::StatusSnapshot status_snapshot() override;
    /** 产品能力，不改变硬件。 */
    system_backend::Capabilities capabilities() override;
    /** 初始化步骤名称，平台静态字符串。 */
    const char* initialization_name(unsigned step) override;
    /** 初始化步骤标题。 */
    i18n::TextId initialization_title(unsigned step) override;
    /** 沿用原探测、等待和超时逻辑。 */
    DataProc::InitializationStatus poll_initialization_step(unsigned, uint32_t, char*, size_t) override;

  private:
    static constexpr std::uint32_t kPeriodMs = 200;
    std::uint32_t last_ms_ = 0;
    bool sent_ = false;
};
