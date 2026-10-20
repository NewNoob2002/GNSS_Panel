#pragma once
#include "SystemDashModel.h"
#include "SystemDashView.h"
#include "Utils/PageManager/PageBase.h"
namespace page {
class SystemDash final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;
    bool on_back_request() override;
    void on_commit_request() override;

  private:
    void show_power(bool visible);
    static void clicked(lv_event_t* event);
    SystemDashView view_{};
    SystemDashModel model_;
    lv_obj_t* last_focus_ = nullptr;
};
} // namespace page
