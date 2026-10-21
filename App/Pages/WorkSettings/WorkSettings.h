#pragma once
#include "Common/RequestFeedback.h"
#include "Utils/PageManager/PageBase.h"
#include "WorkSettingsModel.h"
#include "WorkSettingsView.h"
namespace page {
class WorkSettings final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;
    void on_commit_request() override;

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
