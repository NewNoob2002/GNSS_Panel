#include "SystemDashView.h"
#include <cassert>
#include "Log.h"
#include "UI/Common/SettingsStyle.h"
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
    lv_obj_set_scrollable(root, false);
    // 与工作页、记录页共用同一网格：状态栏下方主卡 88×77，右侧两行 23 px，底行主操作 138 px + 图标返回 42 px。
    hero_ = style::box(root, 8, 28, 88, 77);
    settings_style::hero(hero_);
    settings_style::icon(hero_, "ui_settings_large", 28, 8, style::kAccent);
    status_dot_ = settings_style::dot(hero_, 73, 8);
    auto* title = caption(hero_, TextId::SystemMenu, 4, 48, 80);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    ui.language = style::box(root, 102, 28, 186, 23, true);
    ui.wifi = style::box(root, 102, 55, 186, 23, true);
    ui.power = style::box(root, 102, 82, 138, 23, true);
    ui.back = style::box(root, 246, 82, 42, 23, true);
    for (auto* row : {ui.language, ui.wifi, ui.power, ui.back})
        style::card(row);
    caption(ui.language, TextId::LanguageTitle, 32, 2, 58);
    wifi_caption_ = caption(ui.wifi, TextId::SystemWifiTitle, 32, 2, 58);
    caption(ui.power, TextId::PowerTitle, 36, 2, 94);
    const auto* font = resource_pool::get_font("oswaldBold_12");
    settings_style::icon(ui.language, "ui_network", 8, 3, style::kInfo);
    wifi_icon_ = settings_style::icon(ui.wifi, "ui_wifi", 8, 3, style::kInfo);
    settings_style::icon(ui.power, "ui_power", 10, 3, style::kDanger);
    settings_style::icon(ui.back, "ui_back", 13, 3);
    language_value_ = style::label(ui.language, font, 94, 2, 84);
    wifi_value_ = style::label(ui.wifi, font, 94, 2, 84);
    for (auto* value : {language_value_, wifi_value_})
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(ui.power, 0), style::kDanger, 0);

    ui.overlay = style::box(root, 0, 0, 294, 126, true);
    lv_obj_set_style_bg_color(ui.overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_70, 0);
    // 状态栏保持可见，确认窗位于其下方。
    auto* panel = style::box(ui.overlay, 8, 30, 278, 90);
    style::panel(panel);
    auto* heading = caption(panel, TextId::PowerTitle, 12, 4, 254);
    lv_obj_set_style_text_font(heading, resource_pool::get_font("oswaldBold_18"), 0);
    auto* hint = caption(panel, TextId::PowerPreviewHint, 12, 32, 254);
    lv_obj_set_style_text_color(hint, style::kMuted, 0);
    // Hold Power on an action until its bar fills; a short press closes the dialog (SystemDash controller).
    ui.shutdown = style::box(panel, 8, 54, 127, 28, true);
    ui.reboot = style::box(panel, 143, 54, 127, 28, true);
    const std::array<TextId, 2> actions{TextId::ShutdownAction, TextId::RebootAction};
    // 与 SaveConfig 一致：关机红、重启橙。未聚焦为描边按钮，聚焦/按住时整块填色，
    // 避免橙色焦点框压在红底上难以分辨。
    const std::array<lv_color_t, 2> tints{style::kDanger, style::kAccent};
    unsigned index = 0;
    for (auto* button : {ui.shutdown, ui.reboot}) {
        const auto tint = tints[index];
        style::card(button);
        lv_obj_set_style_border_color(button, tint, 0);
        lv_obj_set_style_text_color(button, tint, 0);
        for (const lv_style_selector_t state :
             {lv_style_selector_t{LV_STATE_FOCUSED}, lv_style_selector_t{LV_STATE_PRESSED}}) {
            lv_obj_set_style_bg_color(button, tint, state);
            lv_obj_set_style_text_color(button, style::kText, state);
        }
        lv_obj_set_style_clip_corner(button, true, 0);
        hold_fill_[index] = style::box(button, 0, 0, 0, 28); // Behind the caption.
        lv_obj_set_style_bg_color(hold_fill_[index], style::kText, 0);
        lv_obj_set_style_bg_opa(hold_fill_[index], LV_OPA_40, 0);
        auto* text = caption(button, actions[index++], 2, 5, 123);
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
        // 文字颜色随按钮状态继承。
        lv_obj_remove_local_style_prop(text, LV_STYLE_TEXT_COLOR, 0);
    }
    if (!reboot_available_) {
        lv_obj_set_hidden(ui.reboot, true);
        lv_obj_set_x(ui.shutdown, 75); // Centered alone.
    }
    show_power(false);
    if (!focus_.bind(root, all_controls()))
        app_fatal("SystemDash", "Focus driver binding failed");
}

void SystemDashView::destroy() {
    unbind();
    stop();
    focus_.unbind();
}

void SystemDashView::sync_focus() {
    focus_.sync();
}

void SystemDashView::resume_focus() {
    focus_.resume();
}

void SystemDashView::appear() const {
    settings_style::reveal(hero_);
    settings_style::reveal(ui.language, 40);
    settings_style::reveal(ui.wifi, 80);
    settings_style::reveal(ui.power, 120);
    settings_style::reveal(ui.back, 160);
}

void SystemDashView::stop() {
    focus_.stop();
    settings_style::stop(status_dot_);
}

bool SystemDashView::bind(lv_subject_t* subject) {
    unbind();
    observer_ = lv_subject_add_observer(subject, on_state, this);
    return observer_ != nullptr;
}

void SystemDashView::unbind() {
    if (observer_) {
        lv_observer_delete(observer_);
        observer_ = nullptr;
    }
}

void SystemDashView::on_state(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<SystemDashView*>(lv_observer_get_user_data(observer));
    self->update(*static_cast<const SystemDashState*>(lv_subject_get_pointer(subject)));
}

SystemDashView::Control SystemDashView::control(const lv_obj_t* object) const {
    if (object == ui.language)
        return Control::Language;
    if (object == ui.wifi)
        return Control::Wifi;
    if (object == ui.power)
        return Control::Power;
    if (object == ui.back)
        return Control::Back;
    if (object == ui.shutdown)
        return Control::Shutdown;
    if (object == ui.reboot)
        return Control::Reboot;
    return Control::None;
}

lv_obj_t* SystemDashView::control_object(Control control) const {
    switch (control) {
    case Control::Language: return ui.language;
    case Control::Wifi: return ui.wifi;
    case Control::Power: return ui.power;
    case Control::Back: return ui.back;
    case Control::Shutdown: return ui.shutdown;
    case Control::Reboot: return ui.reboot;
    case Control::None: return nullptr;
    }
    return nullptr;
}

void SystemDashView::apply_language() {
    for (unsigned i = 0; i < caption_count_; ++i)
        lv_label_set_text(captions_[i].label, i18n::text(captions_[i].id));
    sync_focus();
}

void SystemDashView::update(const SystemDashState& state) const {
    settings_style::activity(status_dot_, state.wifi_pending,
                             state.wifi_pending ? style::kAccent
                             : state.wifi_on    ? style::kSuccess
                                                : style::kMuted);
    settings_style::set_text(language_value_, i18n::text(state.language == i18n::Language::English
                                                             ? TextId::EnglishName
                                                             : TextId::RussianName));
    // 平台不允许切换时该行只读：不进焦点组，标题与图标转为次文字色，但开关状态保持清晰可读，
    // 不再整行半透明（否则"不可操作"会被误读为"不可用/关闭"）。
    if (lv_obj_is_disabled(ui.wifi) == state.wifi_available) {
        if (state.wifi_available)
            lv_obj_set_disabled(ui.wifi, false);
        else
            lv_obj_set_disabled(ui.wifi, true);
    }
    settings_style::set_text_color(wifi_caption_, state.wifi_available ? style::kText : style::kMuted);
    settings_style::set_recolor(wifi_icon_, state.wifi_available ? style::kInfo : style::kMuted);
    settings_style::set_text(wifi_value_, state.wifi_pending
                                              ? "..."
                                              : i18n::text(state.wifi_on ? TextId::StatusOn : TextId::StatusOff));
}

void SystemDashView::set_hold(lv_obj_t* button, int32_t percent) const {
    percent = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    for (unsigned i = 0; i < 2; ++i) {
        const bool active = button == (i == 0 ? ui.shutdown : ui.reboot);
        const auto width =
            active ? static_cast<int32_t>(lv_obj_get_width(lv_obj_get_parent(hold_fill_[i])) * percent / 100)
                   : int32_t{0};
        if (lv_obj_get_width(hold_fill_[i]) != width)
            lv_obj_set_width(hold_fill_[i], width);
    }
}

void SystemDashView::show_power(bool visible) {
    focus_.stop();
    power_open_ = visible;
    set_hold(nullptr, 0);
    for (auto* action : {ui.shutdown, ui.reboot})
        lv_obj_set_pressed(action, false); // A completed hold leaves the page while still pressed.
    if (visible) {
        lv_obj_set_hidden(ui.overlay, false);
        style::appear(lv_obj_get_child(ui.overlay, 0));
    } else
        lv_obj_set_hidden(ui.overlay, true);
}

std::array<lv_obj_t*, 4> SystemDashView::controls() const {
    return power_open_
               ? std::array<lv_obj_t*, 4>{ui.shutdown, reboot_available_ ? ui.reboot : nullptr, nullptr, nullptr}
               : std::array<lv_obj_t*, 4>{ui.language, ui.wifi, ui.power, ui.back};
}

std::array<lv_obj_t*, 6> SystemDashView::all_controls() const {
    return {ui.language, ui.wifi, ui.power, ui.back, ui.shutdown, ui.reboot};
}
