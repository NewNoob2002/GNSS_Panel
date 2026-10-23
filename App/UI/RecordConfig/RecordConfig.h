#pragma once

#include <memory>
#include "PageBase.h"
#include "RecordConfigModel.h"
#include "RecordConfigView.h"

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
    void toggle();
    static void clicked(lv_event_t* event);
    RecordConfigView view_{};
    std::unique_ptr<RecordConfigModel> model_;
    lv_timer_t* timer_ = nullptr;
};
} // namespace page
