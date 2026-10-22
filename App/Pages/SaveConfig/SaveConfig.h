#pragma once

#include "SaveConfigModel.h"

#include "SaveConfigView.h"
#include "PageManager/PageBase.h"

namespace page {
/** Power-off progress. A user action is committed when the progress ends (cancellable before); a platform
 * shutdown only shows its cause. Leaving for Startup follows the platform's standby report.
 */
class SaveConfig final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewDidAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;
    void onLanguageChanged() override;
    bool onBackRequest() override;

  private:
    void apply_text();
    void cancel_timer();
    static void on_timeout(lv_timer_t* timer);
    SaveConfigView view_{};
    SaveConfigModel model_;
    lv_timer_t* timer_ = nullptr;
};
} // namespace page
