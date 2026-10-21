#pragma once

#include "StartupModel.h"

#include "StartupView.h"
#include "Utils/PageManager/PageBase.h"

namespace page {
/** Startup hold: shows progress and requests power-on; the platform decides and P4App navigates. */
class Startup final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_did_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;
    bool wants_power_hold() const override {
        return true; // Power-on is a 2 s hold.
    }

  private:
    void cancel_timer();
    void cancel_status_timer();
    static void on_status_timeout(lv_timer_t* timer);
    static void set_progress(void* obj, int32_t value);
    static void hold_event(lv_event_t* event);
    static void on_timeout(lv_timer_t* timer);
    StartupView view_{};
    StartupModel model_;
    lv_timer_t* timer_ = nullptr;
    lv_timer_t* status_timer_ = nullptr;
};
} // namespace page
