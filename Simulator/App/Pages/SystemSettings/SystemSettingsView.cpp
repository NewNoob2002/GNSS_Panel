#include "SystemSettingsView.h"
#include <cassert>
#include "Utils/PageStyle.h"
using namespace page;
using i18n::TextId;

lv_obj_t* SystemSettingsView::caption(lv_obj_t* parent, TextId id, int x, int y, int width) {
    assert(caption_count_ < captions_.size());
    auto* label = style::label(parent, resource_pool::get_font("oswaldBold_12"), x, y, width);
    captions_[caption_count_++] = {label, id};
    lv_label_set_text(label, i18n::text(id));
    return label;
}
void SystemSettingsView::create(lv_obj_t* root) {
    caption_count_ = 0;
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    // The StatusBar is hidden for the entire SystemDash flow, so use the full 126px viewport.
    caption(root, TextId::SettingsTitle, 10, 6, 274);
    ui.language = style::box(root, 8, 28, 278, 28, true);
    ui.wifi = style::box(root, 8, 60, 278, 28, true);
    ui.back = style::box(root, 8, 92, 278, 28, true);
    const std::array<TextId, 3> ids{TextId::LanguageTitle, TextId::SystemWifiTitle, TextId::Back};
    unsigned index = 0;
    for (auto* row : {ui.language, ui.wifi, ui.back}) {
        style::card(row);
        caption(row, ids[index++], 8, 5, 98);
    }
    const auto* font = resource_pool::get_font("oswaldBold_12");
    language_value_ = style::label(ui.language, font, 110, 5, 158);
    wifi_value_ = style::label(ui.wifi, font, 110, 5, 158);
    for (auto* value : {language_value_, wifi_value_})
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_add_state(ui.wifi, LV_STATE_DISABLED);
    lv_obj_set_style_opa(ui.wifi, LV_OPA_50, LV_STATE_DISABLED);
}
void SystemSettingsView::apply_language() const {
    for (unsigned i = 0; i < caption_count_; ++i)
        lv_label_set_text(captions_[i].label, i18n::text(captions_[i].id));
}
void SystemSettingsView::update(const DataProc::SystemState& state) {
    lv_label_set_text(language_value_, i18n::text(state.language == i18n::Language::English ? TextId::EnglishName
                                                                                            : TextId::RussianName));
    lv_label_set_text(wifi_value_, i18n::text(state.wifi_on ? TextId::StatusOn : TextId::StatusOff));
}
