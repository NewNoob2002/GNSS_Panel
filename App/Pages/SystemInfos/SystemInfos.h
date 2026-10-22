#pragma once

#include "SystemInfosModel.h"

#include "SystemInfosView.h"
#include "PageManager/PageBase.h"

namespace page {
class SystemInfos final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;
    void onLanguageChanged() override;

  private:
    static void clicked(lv_event_t* event);
    SystemInfosView view_{};
    SystemInfosModel model_;
};
} // namespace page
