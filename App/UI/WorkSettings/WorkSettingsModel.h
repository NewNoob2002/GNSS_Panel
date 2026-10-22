#pragma once
#include <functional>
#include <memory>
#include "DataProc/DataProc.h"

namespace page {
/** Page-owned DataNode and received data. Init on view load, Deinit on unload, LVGL thread only. */
class WorkSettingsModel {
  public:
    bool init();
    void deinit();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {}) const;
    /** Apply a work mode with the roller selection (option indexes). */
    bool apply_work(DataProc::WorkTarget target, unsigned radio_protocol, unsigned radio_channel) const;
    const DataProc::StatusSnapshot& status() const {
        return status_;
    }
    const DataProc::SystemState& settings() const {
        return state_;
    }
    const DataProc::DeviceSnapshot& device() const {
        return device_;
    }
    /** Called on the LVGL thread after published System or Device data was received. */
    void set_change_callback(std::function<void()> callback) {
        change_cb_ = std::move(callback);
    }

  private:
    static int on_event(DataNode* account, DataNode::EventParam_t* event);
    std::unique_ptr<DataNode> account_;
    DataProc::StatusSnapshot status_{};
    DataProc::SystemState state_{};
    DataProc::DeviceSnapshot device_{};
    std::function<void()> change_cb_;
};
} // namespace page
