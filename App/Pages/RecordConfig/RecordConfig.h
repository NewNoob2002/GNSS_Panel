#pragma once
#include "Common/RequestFeedback.h"
#include "RecordConfigModel.h"
#include "RecordConfigView.h"
#include "PageManager/PageBase.h"
namespace page {
class RecordConfig final : public PageBase {
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
    void toggle();
    static void clicked(lv_event_t* event);
    RecordConfigView view_{};
    RecordConfigModel model_;
    RequestFeedback feedback_{};
    lv_timer_t* timer_ = nullptr;
};
} // namespace page
