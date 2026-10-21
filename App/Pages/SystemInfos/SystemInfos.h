#pragma once

#include "SystemInfosModel.h"

#include "SystemInfosView.h"
#include "Utils/PageManager/PageBase.h"

namespace page {
class SystemInfos final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;

  private:
    static void clicked(lv_event_t* event);
    SystemInfosView view_{};
    SystemInfosModel model_;
};
} // namespace page
