#include "SystemDashView.h"
#include <cassert>
#include "Utils/SettingsStyle.h"
using namespace page;
using i18n::TextId;

lv_obj_t* SystemDashView::caption(lv_obj_t* parent, TextId id, int x, int y, int width) {
    assert(caption_count_ < captions_.size());
    auto* label = style::label(parent, resource_pool::get_font("oswaldBold_12"), x, y, width);
    captions_[caption_count_++] = {label, id};
    lv_label_set_text(label, i18n::text(id));
    return label;
}
void SystemDashView::create(lv_obj_t* root, bool reboot_available) {
    caption_count_ = 0;
    reboot_available_ = reboot_available;
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    hero_ = style::box(root, 8, 4, 88, 118);
    settings_style::hero(hero_);
    settings_style::icon(hero_, "ui_settings_large", 28, 30, style::kAccent);
    status_dot_ = settings_style::dot(hero_, 73, 8);
    auto* title = caption(hero_, TextId::SystemMenu, 4, 76, 80);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    ui.language = style::box(root, 102, 4, 186, 32, true);
    ui.wifi = style::box(root, 102, 42, 186, 32, true);
    ui.power = style::box(root, 102, 80, 89, 32, true);
    ui.back = style::box(root, 199, 80, 89, 32, true);
    const std::array<TextId, 4> ids{TextId::LanguageTitle, TextId::SystemWifiTitle, TextId::PowerTitle, TextId::Back};
    unsigned index = 0;
    for (auto* row : {ui.language, ui.wifi, ui.power, ui.back}) {
        style::card(row);
        caption(row, ids[index++], 30, 7, row == ui.language || row == ui.wifi ? 58 : 53);
    }
    const auto* font = resource_pool::get_font("oswaldBold_12");
    settings_style::icon(ui.language, "ui_network", 8, 8, style::kInfo);
    settings_style::icon(ui.wifi, "ui_wifi", 8, 8, style::kInfo);
    settings_style::icon(ui.power, "ui_power", 8, 8, style::kDanger);
    settings_style::icon(ui.back, "ui_back", 8, 8);
    language_value_ = style::label(ui.language, font, 94, 7, 84);
    wifi_value_ = style::label(ui.wifi, font, 94, 7, 84);
    for (auto* value : {language_value_, wifi_value_})
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(ui.power, 0), style::kDanger, 0);

    ui.overlay = style::box(root, 0, 0, 294, 126, true);
    lv_obj_set_style_bg_color(ui.overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_70, 0);
    auto* panel = style::box(ui.overlay, 8, 10, 278, 106);
    style::panel(panel);
    caption(panel, TextId::PowerTitle, 12, 6, 254);
    auto* hint = caption(panel, TextId::PowerPreviewHint, 12, 36, 254);
    lv_obj_set_style_text_color(hint, style::kMuted, 0);
    // Hold Power on an action until its bar fills; a short press closes the dialog (SystemDash controller).
    ui.shutdown = style::box(panel, 8, 68, 127, 28, true);
    ui.reboot = style::box(panel, 143, 68, 127, 28, true);
    const std::array<TextId, 2> actions{TextId::ShutdownAction, TextId::RebootAction};
    index = 0;
    for (auto* button : {ui.shutdown, ui.reboot}) {
        style::card(button);
        // Stay red when focused/held (card() darkens those states); the orange border still marks focus.
        for (const lv_style_selector_t state :
             {lv_style_selector_t{0}, lv_style_selector_t{LV_STATE_FOCUSED}, lv_style_selector_t{LV_STATE_PRESSED}})
            lv_obj_set_style_bg_color(button, style::kDanger, state);
        lv_obj_set_style_clip_corner(button, true, 0);
        hold_fill_[index] = style::box(button, 0, 0, 0, 28); // Behind the caption.
        lv_obj_set_style_bg_color(hold_fill_[index], style::kText, 0);
        lv_obj_set_style_bg_opa(hold_fill_[index], LV_OPA_40, 0);
        auto* text = caption(button, actions[index++], 2, 5, 123);
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    }
    if (!reboot_available_) {
        lv_obj_add_flag(ui.reboot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(ui.shutdown, 75); // Centered alone.
    }
    show_power(false);
}
void SystemDashView::appear() const {
    settings_style::reveal(hero_);
    settings_style::reveal(ui.language, 40);
    settings_style::reveal(ui.wifi, 80);
    settings_style::reveal(ui.power, 120);
    settings_style::reveal(ui.back, 160);
}
void SystemDashView::stop() const {
    settings_style::stop(status_dot_);
}
void SystemDashView::apply_language() const {
    for (unsigned i = 0; i < caption_count_; ++i)
        lv_label_set_text(captions_[i].label, i18n::text(captions_[i].id));
}
void SystemDashView::update(const DataProc::SystemState& state) const {
    settings_style::activity(status_dot_, state.wifi_pending,
                             state.wifi_pending ? style::kAccent
                             : state.wifi_on    ? style::kSuccess
                                                : style::kMuted);
    lv_label_set_text(language_value_, i18n::text(state.language == i18n::Language::English ? TextId::EnglishName
                                                                                            : TextId::RussianName));
    if (state.wifi_available)
        lv_obj_clear_state(ui.wifi, LV_STATE_DISABLED);
    else
        lv_obj_add_state(ui.wifi, LV_STATE_DISABLED);
    lv_obj_set_style_opa(ui.wifi, LV_OPA_50, LV_STATE_DISABLED);
    lv_label_set_text(wifi_value_,
                      state.wifi_pending ? "..." : i18n::text(state.wifi_on ? TextId::StatusOn : TextId::StatusOff));
}
void SystemDashView::set_hold(lv_obj_t* button, int32_t percent) const {
    percent = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    for (unsigned i = 0; i < 2; ++i) {
        const bool active = button == (i == 0 ? ui.shutdown : ui.reboot);
        const auto width =
            active ? static_cast<lv_coord_t>(lv_obj_get_width(lv_obj_get_parent(hold_fill_[i])) * percent / 100)
                   : lv_coord_t{0};
        if (lv_obj_get_width(hold_fill_[i]) != width)
            lv_obj_set_width(hold_fill_[i], width);
    }
}
void SystemDashView::show_power(bool visible) {
    power_open_ = visible;
    set_hold(nullptr, 0);
    for (auto* action : {ui.shutdown, ui.reboot})
        lv_obj_clear_state(action, LV_STATE_PRESSED); // A completed hold leaves the page while still pressed.
    if (visible) {
        lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
        style::appear(lv_obj_get_child(ui.overlay, 0));
    } else
        lv_obj_add_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
}
std::array<lv_obj_t*, 4> SystemDashView::controls() const {
    return power_open_
               ? std::array<lv_obj_t*, 4>{ui.shutdown, reboot_available_ ? ui.reboot : nullptr, nullptr, nullptr}
               : std::array<lv_obj_t*, 4>{ui.language, ui.wifi, ui.power, ui.back};
}
std::array<lv_obj_t*, 6> SystemDashView::all_controls() const {
    return {ui.language, ui.wifi, ui.power, ui.back, ui.shutdown, ui.reboot};
}
