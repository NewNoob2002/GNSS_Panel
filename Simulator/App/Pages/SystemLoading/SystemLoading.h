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
    /** " [DEMO]" in logs only when the platform reports simulated data. */
    const char* demo_tag() const {
        return model_.settings().simulated ? " [DEMO]" : "";
    }
    static void on_timeout(lv_timer_t* timer);
    /** Initialization phase tick: poll the running step, then advance once it has a result. */
    void poll_initialization(lv_timer_t* timer);
    void begin_step(unsigned step);
    // Steps take their real check time; these only keep each headline and result readable.
    static constexpr uint32_t kPollMs = 50, kMinStepMs = 300, kResultMs = 150;
    SystemLoadingView view_{};
    SystemLoadingModel model_;
    enum class Phase { Logo, Initialization, Completion, Ready };
    lv_timer_t* timer_ = nullptr;
    Phase phase_ = Phase::Logo;
    unsigned step_ = 0;
    DataProc::InitializationStatus step_status_ = DataProc::InitializationStatus::NotRun;
    uint32_t step_started_ = 0, step_finished_ = 0;
};
} // namespace page
