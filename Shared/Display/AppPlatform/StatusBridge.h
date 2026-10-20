#pragma once
#include <cstdint>

class P4App;

/** systemInfo -> 状态栏快照，固定周期发布（含首次）。 */
class StatusBridge {
  public:
    void update(P4App& app, std::uint32_t now);

  private:
    static constexpr std::uint32_t kPeriodMs = 200;
    std::uint32_t last_ms_ = 0;
    bool sent_ = false;
};
