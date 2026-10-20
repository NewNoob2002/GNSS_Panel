#pragma once

#include "SystemLoadingModel.h"

#include "SystemLoadingView.h"
#include "Utils/PageManager/PageBase.h"

namespace page {
/** Loading animation and preview transition, not a hardware-check model. */
class SystemLoading final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_did_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;

  private:
    void cancel_timer();
    static void on_timeout(lv_timer_t* timer);
    SystemLoadingView view_{};
    SystemLoadingModel model_;
    enum class Phase { Logo, Initialization, Completion, Ready };
    lv_timer_t* timer_ = nullptr;
    Phase phase_ = Phase::Logo;
    unsigned step_ = 0;
};
} // namespace page
