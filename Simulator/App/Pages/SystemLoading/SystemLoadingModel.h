#pragma once
#include <memory>
#include "Common/DataProc/DataProc.h"

namespace page {
/** Page-owned Account and received data. Init on view load, Deinit on unload, LVGL thread only. */
class SystemLoadingModel {
  public:
    bool init();
    void deinit();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {}) const;
    /** UI thread: clear the retained report when a new initialization attempt starts. */
    bool begin_initialization() const;
    /** UI thread: run initialization step 0..4 through system_backend and record it in System.
     * False marks a failure that is shown and logged, never a gate for the next step.
     */
    bool initialize_step(unsigned step) const;
    const DataProc::StatusSnapshot& status() const {
        return status_;
    }
    const DataProc::SystemState& settings() const {
        return state_;
    }

  private:
    static int on_event(Account* account, Account::EventParam_t* event);
    std::unique_ptr<Account> account_;
    DataProc::StatusSnapshot status_{};
    DataProc::SystemState state_{};
};
} // namespace page
