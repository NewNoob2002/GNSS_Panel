#pragma once

#include "Pages/StatusBar/StatusBar.h"

#include <cstdint>

/** Deterministic placeholder status producer used until the real backend exists. */
class DemoStatus {
  public:
    /**
     * Build a synthetic status snapshot from monotonic elapsed milliseconds.
     * The result contains no hardware or business data and is safe to copy.
     */
    static page::StatusBarState sample(std::uint64_t elapsed_ms);

    /** Host 随机预览：每 3 秒生成一组状态，相同 seed/时间可复现；由 LVGL 线程发布。 */
    static page::StatusBarState sample_random(std::uint64_t elapsed_ms, std::uint32_t seed);
};
