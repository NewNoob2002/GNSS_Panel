#pragma once

#include "RecordConfigModel.h"

#include <array>
#include "RecordConfigView.h"
#include "Utils/PageManager/PageBase.h"

namespace page {
/** Recording-option preview and navigation; record remains disabled without its backend. */
class RecordConfig final : public PageBase {
  public:
    void on_custom_attr_config() override;
    void on_view_load() override;
    void on_view_will_appear() override;
    void on_view_will_disappear() override;
    void on_view_unload() override;
    void on_language_changed() override;

  private:
    std::array<lv_obj_t*, 6> controls() const;
    bool recording() const;
    /** Re-apply recording state: button image, roller lock and the focus order. */
    void refresh();
    void on_commit_request() override;
    static void clicked(lv_event_t* event);
    RecordConfigView view_{};
    RecordConfigModel model_;
    lv_obj_t* last_focus_ = nullptr;
};
} // namespace page
