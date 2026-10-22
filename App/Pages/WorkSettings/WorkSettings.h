#pragma once
#include "Common/RequestFeedback.h"
#include "PageManager/PageBase.h"
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
    void refresh();
    void apply();
    static void clicked(lv_event_t* event);
    WorkSettingsView view_{};
    WorkSettingsModel model_;
    DataProc::WorkTarget target_{};
    RequestFeedback feedback_{};
    lv_timer_t* timer_ = nullptr;
};
} // namespace page
