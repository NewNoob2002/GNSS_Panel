#pragma once
#include <functional>
#include <memory>
#include "DataProc/DataProc.h"

namespace page {
/** Page-owned DataNode and received data. Init on view load, Deinit on unload, LVGL thread only. */
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
    /** Observe accepted System publications while the page is visible. */
    void set_change_callback(std::function<void()> callback) {
        change_cb_ = std::move(callback);
    }
    bool finish_power();
    bool cancel_power();

  private:
    std::function<void()> change_cb_;
    static int on_event(DataNode* account, DataNode::EventParam_t* event);
    std::unique_ptr<DataNode> account_;
    DataProc::StatusSnapshot status_{};
    DataProc::SystemState state_{};
};
} // namespace page
