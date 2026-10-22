#pragma once
#include "SystemDashModel.h"
#include "SystemDashView.h"
#include "PageManager/PageBase.h"
namespace page {
class SystemDash final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewWillDisappear() override;
    void onViewDidDisappear() override;
    void onViewUnload() override;
    void onLanguageChanged() override;
    bool onBackRequest() override;
    /** The power confirmation takes a Power hold (to 100%) instead of click gestures. */
    bool wantsPowerHold() const override {
        return view_.power_open();
    }

  private:
    static constexpr uint32_t kHoldMs = 2000; // Same hold as power-on.
    void show_power(bool visible);
    void cancel_hold();
    static void clicked(lv_event_t* event);
    static void hold_event(lv_event_t* event);
    static void on_hold_complete(lv_timer_t* timer);
    static void set_hold_progress(void* self, int32_t percent);
    SystemDashView view_{};
    SystemDashModel model_;
    lv_obj_t* last_focus_ = nullptr;
    lv_obj_t* holding_ = nullptr; // Action button being held.
    lv_timer_t* hold_timer_ = nullptr;
};
} // namespace page
