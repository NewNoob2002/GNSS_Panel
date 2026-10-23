#pragma once

#include <memory>
#include "PageBase.h"
#include "WorkSettingsModel.h"
#include "WorkSettingsView.h"

namespace page {
class WorkSettings final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;
    void onLanguageChanged() override;
    void onCommitRequest() override;

  private:
    void apply();
    static void clicked(lv_event_t* event);
    WorkSettingsView view_{};
    std::unique_ptr<WorkSettingsModel> model_;
    lv_timer_t* timer_ = nullptr;
};
} // namespace page
