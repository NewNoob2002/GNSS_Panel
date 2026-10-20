#pragma once
#include <memory>
#include "Common/DataProc/DataProc.h"

namespace page {
/** Page-owned Account and received data. Init on view load, Deinit on unload, LVGL thread only. */
class SaveConfigModel {
  public:
    bool init();
    void deinit();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {}) const;
    const DataProc::StatusSnapshot& status() const {
        return status_;
    }
    const DataProc::SystemState& settings() const {
        return state_;
    }
    bool finish_power();
    bool cancel_power();

  private:
    static int on_event(Account* account, Account::EventParam_t* event);
    std::unique_ptr<Account> account_;
    DataProc::StatusSnapshot status_{};
    DataProc::SystemState state_{};
};
} // namespace page
