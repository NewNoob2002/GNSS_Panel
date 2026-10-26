#include "SystemDashView.h"
#include <algorithm>
#include <cassert>
#include "Log.h"
#include "UI/Common/SettingsStyle.h"
#include "UI/Common/PageUtils.h"
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
    wifi_available_ = false;
    lv_obj_set_scrollable(root, false);
    // 与工作/记录设置共用主卡与右侧三行网格，能力变化不改变几何。
    ui.brightness = style::box(root, 8, 28, 88, 77, true);
    settings_style::hero(ui.brightness);
    settings_style::icon(ui.brightness, "ui_sun_large", 28, 3, style::kAccent);
    auto* brightness_title = caption(ui.brightness, TextId::BrightnessTitle, 4, 35, 80);
    lv_obj_set_style_text_align(brightness_title, LV_TEXT_ALIGN_CENTER, 0);
    // 五段共 66 px，在主卡内居中，标题与焦点边框之间保留留白。
    for (unsigned i = 0; i < brightness_segments_.size(); ++i) {
        auto* segment = style::box(ui.brightness, (static_cast<int>(i) - 2) * 14, 25, 10, 4);
        lv_obj_set_align(segment, LV_ALIGN_CENTER);
        lv_obj_set_style_radius(segment, 2, 0);
        lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(segment, style::kAccent, 0);
        brightness_segments_[i] = segment;
    }

    ui.language = style::box(root, 102, 28, 186, 23, true);
    ui.wifi = style::box(root, 102, 55, 186, 23, true);
    ui.power = style::box(root, 102, 82, 138, 23, true);
    ui.back = style::box(root, 246, 82, 42, 23, true);
    for (auto* row : {ui.language, ui.wifi, ui.power, ui.back})
        style::control(row);
    caption(ui.language, TextId::LanguageTitle, 32, 2, 58);
    wifi_caption_ = caption(ui.wifi, TextId::SystemWifiTitle, 32, 2, 58);
    caption(ui.power, TextId::PowerTitle, 36, 2, 94);
    settings_style::icon(ui.language, "ui_network", 8, 3, style::kInfo);
    wifi_icon_ = settings_style::icon(ui.wifi, "ui_wifi", 8, 3, style::kMuted);
    settings_style::icon(ui.power, "ui_power", 10, 3, style::kDanger);
    settings_style::icon(ui.back, "ui_back", 13, 3);
    const auto* font = resource_pool::get_font("oswaldBold_12");
    language_value_ = style::label(ui.language, font, 98, 2, 80);
    wifi_value_ = style::label(ui.wifi, font, 98, 2, 80);
    for (auto* value : {language_value_, wifi_value_})
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(ui.power, 0), style::kDanger, 0);
    lv_obj_set_disabled(ui.wifi, true);
    settings_style::set_visible(ui.wifi, true);

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
    const std::array<lv_color_t, 2> tints{style::kDanger, style::kAccent};
    static lv_style_transition_dsc_t transition;
    static constexpr lv_style_prop_t properties[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};
    static bool initialized = false;
    if (!initialized) {
        lv_style_transition_dsc_init(&transition, properties, lv_anim_path_ease_out, 140, 0, nullptr);
        initialized = true;
    }
    unsigned index = 0;
    for (auto* button : {ui.shutdown, ui.reboot}) {
        const auto tint = tints[index];
        style::card(button);
        lv_obj_set_style_radius(button, 7, 0);
        lv_obj_set_style_bg_color(button, lv_color_mix(tint, style::kCard, 20), 0);
        lv_obj_set_style_border_color(button, lv_color_mix(tint, style::kBorder, 90), 0);
        lv_obj_set_style_text_color(button, lv_color_mix(style::kText, style::kMuted, 160), 0);
        for (const lv_style_selector_t state :
             {lv_style_selector_t{0}, lv_style_selector_t{LV_STATE_FOCUSED}, lv_style_selector_t{LV_STATE_PRESSED}})
            lv_obj_set_style_transition(button, &transition, state);
        lv_obj_set_style_bg_color(button, lv_color_mix(tint, style::kCard, 48), LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(button, lv_color_mix(tint, style::kCard, 72), LV_STATE_PRESSED);
        lv_obj_set_style_text_color(button, style::kText, LV_STATE_FOCUSED);
        lv_obj_set_style_text_color(button, style::kText, LV_STATE_PRESSED);
        auto* track = style::box(button, 0, 0, lv_pct(100), 3);
        lv_obj_set_align(track, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_y(track, -2);
        lv_obj_set_style_radius(track, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(track, lv_color_mix(tint, style::kCard, 55), 0);
        lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
        hold_fill_[index] = style::box(track, 0, 0, 0, lv_pct(100));
        lv_obj_set_style_radius(hold_fill_[index], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(hold_fill_[index], tint, 0);
        lv_obj_set_style_bg_opa(hold_fill_[index], LV_OPA_COVER, 0);
        auto* text = caption(button, actions[index++], 0, 0, lv_pct(100));
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_remove_local_style_prop(text, LV_STYLE_TEXT_COLOR, 0);
    }
    if (!reboot_available_) {
        lv_obj_set_hidden(ui.reboot, true);
        lv_obj_set_x(ui.shutdown, 75); // Centered alone.
    }
    show_power(false);
    if (!focus_.bind(root, all_controls()))
        app_fatal("SystemDash", "Focus driver binding failed");
    // 电源动作使用自身细边框，保留焦点驱动但不叠加挤压边框。
    for (unsigned i = 0; i < 2; ++i) {
        auto* button = i == 0 ? ui.shutdown : ui.reboot;
        for (const lv_style_selector_t state : {lv_style_selector_t{0}, lv_style_selector_t{LV_STATE_FOCUSED}}) {
            lv_obj_set_style_pad_hor(button, 7, state);
            lv_obj_set_style_pad_ver(button, 2, state);
        }
        lv_obj_set_style_border_color(button, tints[i], LV_STATE_FOCUSED);
    }
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
    uint32_t delay = 0;
    for (auto* row : {ui.brightness, ui.language, ui.wifi, ui.power, ui.back}) {
        if (!lv_obj_is_hidden(row)) {
            settings_style::reveal(row, delay);
            delay += 40;
        }
    }
}

void SystemDashView::stop() {
    focus_.stop();
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
    if (object == ui.brightness)
        return Control::Brightness;
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
    case Control::Brightness: return ui.brightness;
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

void SystemDashView::update(const SystemDashState& state) {
    const unsigned level = std::clamp((static_cast<unsigned>(state.brightness) + 10) / 20, 1u, 5u);
    const auto dim = lv_color_mix(style::kAccent, style::kCard, 48);
    for (unsigned i = 0; i < brightness_segments_.size(); ++i) {
        auto* segment = brightness_segments_[i];
        const auto color = i < level ? style::kAccent : dim;
        if (!lv_color_eq(lv_obj_get_style_bg_color(segment, LV_PART_MAIN), color))
            lv_obj_set_style_bg_color(segment, color, 0);
    }
    settings_style::set_text(language_value_, i18n::text(state.language == i18n::Language::English
                                                             ? TextId::EnglishName
                                                             : TextId::RussianName));
    if (wifi_available_ != state.wifi_available) {
        auto* group = lv_group_get_default();
        auto* preferred = group ? lv_group_get_focused(group) : nullptr;
        // 初次绑定及离页更新不能接管其他页面的焦点；确认窗内保留动作焦点。
        const bool owns_group = group && lv_obj_get_group(ui.brightness) == group;
        wifi_available_ = state.wifi_available;
        lv_obj_set_disabled(ui.wifi, !wifi_available_);
        if (owns_group) {
            if (preferred == ui.wifi && !wifi_available_)
                preferred = ui.brightness;
            utils::focus_controls(controls(), preferred);
            sync_focus();
        }
    }
    settings_style::set_text_color(wifi_caption_, wifi_available_ ? style::kText : style::kMuted);
    settings_style::set_recolor(wifi_icon_, wifi_available_ ? style::kInfo : style::kMuted);
    settings_style::set_text_color(wifi_value_, !wifi_available_ || !state.wifi_on ? style::kMuted : style::kSuccess);
    settings_style::set_text(wifi_value_, state.wifi_pending && wifi_available_
                                              ? "..."
                                              : i18n::text(state.wifi_on ? TextId::StatusOn : TextId::StatusOff));
}

void SystemDashView::set_hold(lv_obj_t* button, int32_t percent) const {
    percent = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    for (unsigned i = 0; i < 2; ++i) {
        const bool active = button == (i == 0 ? ui.shutdown : ui.reboot);
        lv_obj_set_width(hold_fill_[i], lv_pct(active ? percent : 0));
    }
}

void SystemDashView::show_power(bool visible) {
    focus_.stop();
    power_open_ = visible;
    if (focus_.object())
        lv_obj_set_style_border_opa(focus_.object(), visible ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    set_hold(nullptr, 0);
    for (auto* action : {ui.shutdown, ui.reboot})
        lv_obj_set_pressed(action, false); // A completed hold leaves the page while still pressed.
    if (visible) {
        lv_obj_set_hidden(ui.overlay, false);
        style::appear(lv_obj_get_child(ui.overlay, 0));
    } else
        lv_obj_set_hidden(ui.overlay, true);
}

std::array<lv_obj_t*, 5> SystemDashView::controls() const {
    return power_open_
               ? std::array<lv_obj_t*, 5>{ui.shutdown, reboot_available_ ? ui.reboot : nullptr, nullptr, nullptr, nullptr}
               : std::array<lv_obj_t*, 5>{ui.brightness, ui.language, wifi_available_ ? ui.wifi : nullptr, ui.power, ui.back};
}

std::array<lv_obj_t*, 7> SystemDashView::all_controls() const {
    return {ui.brightness, ui.language, ui.wifi, ui.power, ui.back, ui.shutdown, ui.reboot};
}
