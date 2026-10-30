#pragma once

#include "DialplateModel.h"

#include <memory>
#include <array>
#include "DialplateView.h"
#include "PageBase.h"
#include "UI/PageId.h"

namespace page {
/** Dashboard navigation and focus lifecycle; View owns visual widgets only. */
class Dialplate final : public PageBase {
  public:
    void onInstalled() override;
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewDidAppear() override;
    void onViewWillDisappear() override;
    void onViewUnload() override;

  private:
    std::array<lv_obj_t*, 5> controls() const;
    static void clicked(lv_event_t* event);
    /** @brief SmoothSelector 式入口：焦点框展开盖住首页后，再以 NONE 推入目标页（返回同样 NONE，再收回）。 */
    void open_entry(PageId destination);
    static void on_entry_open(void* user_data);
    static void navigate_entry(void* user_data);
    static void close_after_frame(lv_event_t* event);
    void cancel_close();
    lv_display_t* close_display_ = nullptr;
    PageId pending_entry_ = PageId::Invalid;
    DialplateView view_{};
    std::unique_ptr<DialplateModel> model_;
    lv_obj_t* last_focus_ = nullptr;
    bool entrance_pending_ = true; // Play the entrance once per load (boot), not on every return.
};
} // namespace page
