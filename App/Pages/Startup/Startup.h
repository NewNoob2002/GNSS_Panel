#pragma once

#include "StartupModel.h"

#include "StartupView.h"
#include "PageBase.h"

namespace page {
/** Startup hold: shows progress and requests power-on; the platform decides and App navigates. */
class Startup final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewDidAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;
    void onLanguageChanged() override;
    bool wantsPowerHold() const override {
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
