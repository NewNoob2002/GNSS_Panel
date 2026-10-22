#pragma once

#include "StarMapModel.h"

#include "StarMapView.h"
#include "PageBase.h"

namespace page {
class StarMap final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;
    void onLanguageChanged() override;

  private:
    static void clicked(lv_event_t* event);
    StarMapView view_{};
    StarMapModel model_;
};
} // namespace page
