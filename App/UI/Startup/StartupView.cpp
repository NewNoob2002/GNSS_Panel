#include <cstdio>
#include "FixedFormat.h"
#include "StartupView.h"
#include "UI/Common/BatteryStyle.h"
#include "UI/Common/SettingsStyle.h"

using namespace page;

namespace {
constexpr int32_t kStatusBarHeight = 26;
constexpr int32_t kBarMaxWidth = 148;
} // namespace

void StartupView::create(lv_obj_t* root) {
    const auto* font_small = resource_pool::get_font("oswaldBold_12");
    const auto* font_main = resource_pool::get_font("oswaldBold_18");
    const auto* font_digits = resource_pool::get_font("rajdhaniBold_40");
    lv_obj_set_style_bg_color(root, style::kBackground, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    // Reserve the header band for the status bar that appears after the idle delay.
    auto* heading = style::label(root, font_main, 8, 0, 224);
    lv_label_set_text(heading, "GNSS RTK");
    auto* badge = style::mark_demo(style::box(root, 244, 4, 42, 18));
    style::card(badge);
    auto* demo = style::label(badge, font_small, 0, 0, 40);
    lv_obj_set_style_text_align(demo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(demo, style::kMuted, 0);
    lv_label_set_text(demo, "DEMO");

    auto* content = style::box(root, 0, kStatusBarHeight, LV_HOR_RES, LV_VER_RES - kStatusBarHeight);
    auto* battery = style::box(content, 12, 6, 156, 94);
    ui.bat_caption = style::label(battery, font_small, 0, 0, 40);
    lv_obj_set_style_text_color(ui.bat_caption, style::kMuted, 0);
    lv_label_set_text(ui.bat_caption, "BAT");
    ui.bat_voltage = style::label(battery, font_small, 60, 0, 92);
    lv_obj_set_style_text_align(ui.bat_voltage, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(ui.bat_voltage, style::kMuted, 0);
    lv_label_set_text(ui.bat_voltage, "N/A");

    auto* reading = style::box(battery, 0, 16, 156, 34);
    lv_obj_set_flex_flow(reading, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(reading, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(reading, 2, 0);
    ui.bat_charge = style::label(reading, font_digits, 0, 0, LV_SIZE_CONTENT);
    // Rajdhani's baseline is six pixels lower than Oswald's at the same line height.
    lv_obj_set_style_pad_bottom(ui.bat_charge, 6, 0);
    lv_label_set_text(ui.bat_charge, "0");
    ui.bat_unit = style::label(reading, font_main, 0, 0, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui.bat_unit, style::kMuted, 0);
    lv_label_set_text(ui.bat_unit, "%");
    ui.bat_fill = battery::create_slot(battery, 0, 52, kBarMaxWidth, 6);

    auto* firmware_caption = style::label(battery, font_small, 0, 72, 18);
    lv_obj_set_style_text_color(firmware_caption, style::kMuted, 0);
    lv_label_set_text(firmware_caption, "FW");
    ui.firmware = style::label(battery, font_small, 22, 72, 56);
    auto* hardware_caption = style::label(battery, font_small, 82, 72, 18);
    lv_obj_set_style_text_color(hardware_caption, style::kMuted, 0);
    lv_label_set_text(hardware_caption, "HW");
    ui.hardware = style::label(battery, font_small, 104, 72, 52);

    ui.btn_press = style::box(content, 178, 2, 110, 96, true);
    lv_obj_set_flex_flow(ui.btn_press, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui.btn_press, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(ui.btn_press, 6, 0);
    lv_obj_set_style_pad_bottom(ui.btn_press, 4, 0);
    lv_obj_set_style_text_color(ui.btn_press, style::kHighlight, LV_STATE_PRESSED);
    ui.arc = style::ring(ui.btn_press, 20, 0, 70, 4, style::kHighlight);
    // Keep the ring above the flex row while both labels remain direct children of the hold target.
    lv_obj_set_floating(ui.arc, true);
    settings_style::icon(ui.arc, "ui_power_large", 19, 10, style::kHighlight);
    ui.arc_percent = style::label(ui.arc, font_small, 8, 46, 54);
    lv_obj_set_style_text_align(ui.arc_percent, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui.arc_percent, style::kMuted, 0);
    lv_label_set_text(ui.arc_percent, "0%");
    ui.btn_label = style::label(ui.btn_press, font_small, 0, 0, LV_SIZE_CONTENT);
    // Inherit the target's pressed color without a timer or an opacity transition.
    lv_obj_remove_local_style_prop(ui.btn_label, LV_STYLE_TEXT_COLOR, 0);
    lv_obj_set_style_text_color(ui.btn_press, style::kText, 0);
    ui.hold_hint = style::label(ui.btn_press, font_small, 0, 0, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui.hold_hint, style::kMuted, 0);

    lv_anim_init(&ui.arc_anim);
    lv_anim_set_var(&ui.arc_anim, this);
    lv_anim_set_duration(&ui.arc_anim, 200);
    lv_anim_set_reverse_duration(&ui.arc_anim, 0);
    lv_anim_set_repeat_count(&ui.arc_anim, 0);
    lv_anim_set_path_cb(&ui.arc_anim, lv_anim_path_linear);
    apply_language();
}

void StartupView::destroy() {
    unbind();
    stop_hold_anim();
    battery::stop(ui.bat_fill);
}

bool StartupView::bind(lv_subject_t* subject) {
    unbind();
    observer_ = lv_subject_add_observer(subject, on_state, this);
    return observer_ != nullptr;
}

void StartupView::unbind() {
    if (observer_) {
        lv_observer_delete(observer_);
        observer_ = nullptr;
    }
}

void StartupView::on_state(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<StartupView*>(lv_observer_get_user_data(observer));
    self->update(*static_cast<const StartupState*>(lv_subject_get_pointer(subject)));
}

void StartupView::anim_cb(void* var, int32_t value) {
    static_cast<StartupView*>(var)->set_hold_progress(value);
}

void StartupView::set_hold_progress(int32_t value) {
    if (ui.arc)
        lv_arc_set_value(ui.arc, static_cast<int16_t>(value));
    if (ui.arc_percent)
        lv_label_set_text_fmt(ui.arc_percent, "%ld%%", static_cast<long>(value));
}

void StartupView::reset_hold() {
    set_hold_progress(0);
    if (ui.btn_press)
        lv_obj_set_pressed(ui.btn_press, false);
}

void StartupView::start_hold_anim(uint32_t hold_ms) {
    if (!ui.arc)
        return;
    lv_anim_delete(this, anim_cb);
    set_hold_progress(0);
    lv_anim_set_var(&ui.arc_anim, this);
    lv_anim_set_exec_cb(&ui.arc_anim, anim_cb);
    lv_anim_set_values(&ui.arc_anim, 0, 100);
    lv_anim_set_duration(&ui.arc_anim, hold_ms);
    lv_anim_start(&ui.arc_anim);
}

void StartupView::reset_hold_anim(uint32_t restore_ms) {
    if (!ui.arc)
        return;
    lv_anim_delete(this, anim_cb);
    const int32_t current = lv_arc_get_value(ui.arc);
    lv_anim_set_var(&ui.arc_anim, this);
    lv_anim_set_exec_cb(&ui.arc_anim, anim_cb);
    lv_anim_set_values(&ui.arc_anim, current, 0);
    lv_anim_set_duration(&ui.arc_anim, restore_ms);
    lv_anim_start(&ui.arc_anim);
}

void StartupView::stop_hold_anim() {
    lv_anim_delete(this, anim_cb);
}

void StartupView::update(const StartupState& state) {
    if (!ui.bat_charge || !ui.bat_fill)
        return;
    const unsigned percent = state.battery_percent > 100 ? 100 : state.battery_percent;
    settings_style::set_visible(ui.bat_charge, state.battery_valid);
    settings_style::set_text(ui.bat_unit, state.battery_valid ? "%" : "N/A");
    if (state.battery_valid) {
        char text[24];
        lv_snprintf(text, sizeof(text), "%u", percent);
        settings_style::set_text(ui.bat_charge, text);
        app::format_fixed(text, sizeof(text), state.battery_voltage, 2, " V");
        settings_style::set_text(ui.bat_voltage, text);
    } else {
        settings_style::set_text(ui.bat_voltage, "N/A");
    }
    const bool charging = state.battery_valid && state.charging;
    const auto color = state.battery_valid ? battery::color(percent, charging) : style::kMuted;
    settings_style::set_text(ui.bat_caption, charging ? "CHG" : "BAT");
    settings_style::set_text_color(ui.bat_caption, charging ? color : style::kMuted);
    battery::update_fill(ui.bat_fill, percent, charging, state.battery_valid);

    settings_style::set_text(ui.firmware, state.firmware[0] ? state.firmware.data() : "N/A");
    settings_style::set_text(ui.hardware, state.hardware[0] ? state.hardware.data() : "N/A");
}

void StartupView::apply_language() const {
    settings_style::set_text(ui.btn_label, i18n::text(i18n::TextId::StartupStart));
    settings_style::set_text(ui.hold_hint, i18n::text(i18n::TextId::StartupHoldHint));
}
