#pragma once
#include <cstdint>
#include <memory>
#include "DataProc/DataProc.h"

namespace page {
/** Page-owned DataNode and received data. Init on view load, Deinit on unload, LVGL thread only. */
class SystemLoadingModel {
  public:
    bool init();
    void deinit();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {}) const;
    /** UI thread: clear the retained report when a new initialization attempt starts. */
    bool begin_initialization() const;
    /** Upper bound for one step, above the platform's own timeouts: a backend that never finishes a step is
     * recorded as failed so startup always reaches the working UI.
     */
    static constexpr std::uint32_t kStepLimitMs = 35000;
    /** UI thread: poll initialization step 0..4 through system_backend (elapsed_ms from its first poll, 0).
     * Returns Pending while it runs; once it has a result, records it in System and returns Ok/Failed.
     * Failed is shown and logged, never a gate for the next step.
     */
    DataProc::InitializationStatus poll_step(unsigned step, std::uint32_t elapsed_ms) const;
    const DataProc::StatusSnapshot& status() const {
        return status_;
    }
    const DataProc::SystemState& settings() const {
        return state_;
    }

  private:
    static int on_event(DataNode* account, DataNode::EventParam_t* event);
    std::unique_ptr<DataNode> account_;
    DataProc::StatusSnapshot status_{};
    DataProc::SystemState state_{};
};
} // namespace page
