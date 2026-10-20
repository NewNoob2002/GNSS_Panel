#pragma once

#include "SaveConfigModel.h"

#include "SaveConfigView.h"
#include "Utils/PageManager/PageBase.h"

namespace page {
/** Power-off progress. A user action is committed when the progress ends (cancellable before); a platform
 * shutdown only shows its cause. Leaving for Startup follows the platform's standby report.
 */
class SaveConfig final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_did_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;
    bool on_back_request() override;

  private:
    void apply_text() const;
    void cancel_timer();
    static void on_timeout(lv_timer_t* timer);
    SaveConfigView view_{};
    SaveConfigModel model_;
    lv_timer_t* timer_ = nullptr;
};
} // namespace page
