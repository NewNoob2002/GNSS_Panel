#pragma once

#include "SystemBackend.h"
#include "UI/StatusBar/StatusBar.h"

#include <cstdint>

/** Deterministic placeholder status producer used until the real backend exists. */
class DemoStatus : public system_backend::StatusObject {
  public:
    /** 按当前预览时间和模式生成状态栏值快照。 */
    DataProc::StatusSnapshot status_snapshot() override;
    /** 演示平台的功能能力。 */
    system_backend::Capabilities capabilities() override;
    /** 演示初始化步骤名称。 */
    const char* initialization_name(unsigned step) override;
    /** 演示初始化步骤标题。 */
    i18n::TextId initialization_title(unsigned step) override;
    /** 保留原演示步骤耗时及失败结果。 */
    DataProc::InitializationStatus poll_initialization_step(unsigned, uint32_t, char*, size_t) override;
    /**
     * Build a synthetic status snapshot from monotonic elapsed milliseconds.
     * The result contains no hardware or business data and is safe to copy.
     */
    static page::StatusBarState sample(std::uint64_t elapsed_ms);

    /** Host 随机预览：每 3 秒生成一组状态，相同 seed/时间可复现；由 LVGL 线程发布。 */
    static page::StatusBarState sample_random(std::uint64_t elapsed_ms, std::uint32_t seed);
};
