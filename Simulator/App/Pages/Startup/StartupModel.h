#pragma once
#include <functional>
#include <memory>
#include "Common/DataProc/DataProc.h"

namespace page {
/** Page-owned Account and received data. Init on view load, Deinit on unload, LVGL thread only. */
class StartupModel {
  public:
    bool init();
    void deinit();
    /** Called on the LVGL thread after published Status or Device data was received. */
    void set_change_callback(std::function<void()> callback) {
        change_cb_ = std::move(callback);
    }
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {}) const;
    /** Hold completed: ask System to power on. Confirmation arrives as SystemState::powered_on. */
    bool request_power_on() const;
    const DataProc::StatusSnapshot& status() const {
        return status_;
    }
    const DataProc::SystemState& settings() const {
        return state_;
    }
    const DataProc::DeviceSnapshot& device() const {
        return device_;
    }

  private:
    static int on_event(Account* account, Account::EventParam_t* event);
    std::unique_ptr<Account> account_;
    DataProc::StatusSnapshot status_{};
    DataProc::SystemState state_{};
    DataProc::DeviceSnapshot device_{};
    std::function<void()> change_cb_;
};
} // namespace page
