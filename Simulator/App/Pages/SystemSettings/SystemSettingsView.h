#pragma once
#include <array>
#include "Common/DataProc/DataProc_Def.h"
#include "Resource/ResourcePool.h"
namespace page {
class SystemSettingsView {
  public:
    void create(lv_obj_t* root);
    void destroy() {}
    void apply_language() const;
    void update(const DataProc::SystemState& state);
    std::array<lv_obj_t*, 2> controls() const {
        return {ui.language, ui.back};
    }
    struct {
        lv_obj_t *language{}, *wifi{}, *back{};
    } ui;

  private:
    struct Caption {
        lv_obj_t* label;
        i18n::TextId id;
    };
    std::array<Caption, 4> captions_{};
    unsigned caption_count_ = 0;
    lv_obj_t *language_value_ = nullptr, *wifi_value_ = nullptr;
    lv_obj_t* caption(lv_obj_t* parent, i18n::TextId id, int x, int y, int width);
};
} // namespace page
