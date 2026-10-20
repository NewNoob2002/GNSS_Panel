#pragma once

#include "DialplateModel.h"

#include <array>
#include "DialplateView.h"
#include "Utils/PageManager/PageBase.h"

namespace page {
/** Dashboard navigation and focus lifecycle; View owns visual widgets only. */
class Dialplate final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;

  private:
    std::array<lv_obj_t*, 5> controls() const;
    static void clicked(lv_event_t* event);
    DialplateView view_{};
    DialplateModel model_;
    lv_obj_t* last_focus_ = nullptr;
};
} // namespace page
