#pragma once

#include "WorkSettingsModel.h"

#include <array>
#include "Utils/PageManager/PageBase.h"
#include "WorkSettingsView.h"

namespace page {
/** Radio-setting preview controller; no radio backend operations are available yet. */
class WorkSettings final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;

  private:
    std::array<lv_obj_t*, 5> controls() const;
    static void clicked(lv_event_t* event);
    WorkSettingsView view_{};
    WorkSettingsModel model_;
    lv_obj_t* last_focus_ = nullptr;
};
} // namespace page
