#pragma once

#include "DialplateModel.h"

#include <array>
#include "DialplateView.h"
#include "PageBase.h"

namespace page {
/** Dashboard navigation and focus lifecycle; View owns visual widgets only. */
class Dialplate final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;

  private:
    std::array<lv_obj_t*, 5> controls() const;
    static void clicked(lv_event_t* event);
    DialplateView view_{};
    DialplateModel model_;
    lv_obj_t* last_focus_ = nullptr;
    bool entrance_pending_ = true; // Play the entrance once per load (boot), not on every return.
};
} // namespace page
