#include "StartupView.h"
#include "Utils/BatteryStyle.h"
#include "Utils/PageStyle.h"

using namespace page;

namespace {
constexpr lv_coord_t kStatusBarHeight = 26;
constexpr lv_color_t kBackground = LV_COLOR_MAKE(0x15, 0x15, 0x13);
constexpr lv_color_t kAmber = LV_COLOR_MAKE(0xFF, 0x93, 0x1E);
constexpr lv_color_t kAmberBright = LV_COLOR_MAKE(0xF6, 0xD0, 0x62);
constexpr lv_color_t kMuted = LV_COLOR_MAKE(0x99, 0x99, 0x99);
constexpr lv_coord_t kBarMaxWidth = 90;

void create_info_card(lv_obj_t* parent, const lv_font_t* font, const char* title, const char* value, int x) {
    auto* card = style::box(parent, x, 3, 78, 44);
    style::card(card);
    auto* caption = style::label(card, font, 8, 4, 62);
    lv_obj_set_style_text_color(caption, kMuted, 0);
    lv_label_set_text(caption, title);
    auto* data = style::label(card, font, 8, 22, 62);
    lv_label_set_text(data, value);
}
} // namespace

void StartupView::create(lv_obj_t* root) {
    const auto* font_small = resource_pool::get_font("oswaldBold_12");
    const auto* font_main = resource_pool::get_font("oswaldBold_18");
    lv_obj_set_style_bg_color(root, kBackground, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    // Reserve the header band for the status bar that appears after the idle delay.
    auto* heading = style::label(root, font_main, 8, 0, 224);
    lv_label_set_text(heading, "GNSS RTK");
    auto* badge = style::mark_demo(style::box(root, 244, 4, 42, 18));
    style::card(badge);
    auto* demo = style::label(badge, font_small, 0, 0, 40);
    lv_obj_set_style_text_align(demo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(demo, kMuted, 0);
    lv_label_set_text(demo, "DEMO");

    auto* content = style::box(root, 0, kStatusBarHeight, LV_HOR_RES, LV_VER_RES - kStatusBarHeight);
    // Demo inventory until target providers supply firmware, board and power measurements.
    create_info_card(content, font_small, "FW", "1.0.0", 8);
    create_info_card(content, font_small, "HW", "J V1.0", 94);
    auto* battery = style::box(content, 8, 55, 164, 42);
    style::card(battery);
    auto* icon = lv_img_create(battery);
    lv_img_set_src(icon, resource_pool::get_image("battery_info"));
    lv_img_set_zoom(icon, LV_IMG_ZOOM_NONE / 2);
    lv_img_set_pivot(icon, 0, 0);
    lv_obj_set_pos(icon, 8, 4);
    lv_obj_set_style_img_recolor(icon, kAmber, 0);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
    ui.bat_icon = icon;
    auto* caption = style::label(battery, font_small, 30, 4, 46);
    lv_obj_set_style_text_color(caption, kMuted, 0);
    lv_label_set_text(caption, "BAT");
    ui.bat_caption = caption;
    auto* charge = style::label(battery, font_main, 108, 0, 48);
    lv_obj_set_style_text_align(charge, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(charge, "85%");
    ui.bat_charge = charge;
    auto* voltage = style::label(battery, font_small, 108, 24, 48);
    lv_obj_set_style_text_align(voltage, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(voltage, kMuted, 0);
    lv_label_set_text(voltage, "7.60 V");
    ui.bat_voltage = voltage;
    ui.bat_fill = battery::create_slot(battery, 8, 27, kBarMaxWidth, 6);
    ui.bat_level = lv_obj_get_parent(ui.bat_fill);

    auto* btn_press = style::box(content, 180, 3, 106, 94, true);
    style::card(btn_press);
    lv_obj_set_style_border_width(btn_press, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn_press, kAmber, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(btn_press, lv_color_hex(0x30271d), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_press, 2, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_press, kAmberBright, LV_STATE_PRESSED);
    ui.btn_press = btn_press;

    auto* arc = lv_arc_create(btn_press);
    lv_obj_set_size(arc, 46, 46);
    lv_obj_set_pos(arc, 30, 4);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_arc_set_bg_angles(arc, 135, 45);
    lv_arc_set_rotation(arc, 90);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x555555), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, kAmberBright, LV_PART_INDICATOR);
    ui.arc = arc;
    auto* power = lv_img_create(btn_press);
    lv_img_set_src(power, resource_pool::get_image("shutdown"));
    lv_obj_set_pos(power, 45, 11);
    lv_obj_set_style_img_recolor(power, kAmberBright, 0);
    lv_obj_set_style_img_recolor_opa(power, LV_OPA_COVER, 0);
    auto* percent = style::label(btn_press, font_small, 11, 29, 84);
    lv_obj_set_style_text_align(percent, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(percent, "0%");
    ui.arc_percent = percent;
    auto* label_btn = style::label(btn_press, font_main, 7, 50, 92);
    lv_obj_set_style_text_align(label_btn, LV_TEXT_ALIGN_CENTER, 0);
    ui.btn_label = label_btn;
    auto* hint = style::label(btn_press, font_small, 7, 76, 92);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(hint, kMuted, 0);
    ui.hold_hint = hint;

    lv_anim_init(&ui.arc_anim);
    lv_anim_set_var(&ui.arc_anim, ui.arc);
    lv_anim_set_time(&ui.arc_anim, 200);
    lv_anim_set_playback_time(&ui.arc_anim, 0);
    lv_anim_set_repeat_count(&ui.arc_anim, 0);
    lv_anim_set_path_cb(&ui.arc_anim, lv_anim_path_linear);
    apply_language();
}

void StartupView::destroy() const {
    battery::stop(ui.bat_fill);
}

void StartupView::update(const DataProc::StatusSnapshot& status) {
    if (!ui.bat_charge || !ui.bat_fill)
        return;
    const unsigned percent = status.battery_percent > 100 ? 100 : status.battery_percent;
    if (status.battery_valid) {
        lv_label_set_text_fmt(ui.bat_charge, "%u%%", percent);
        lv_label_set_text_fmt(ui.bat_voltage, "%.2f V", status.battery_voltage);
    } else {
        lv_label_set_text(ui.bat_charge, "N/A");
        lv_label_set_text(ui.bat_voltage, "N/A");
    }
    const bool charging = status.battery_valid && status.charging;
    const auto color = status.battery_valid ? battery::color(percent, charging) : kMuted;
    lv_label_set_text(ui.bat_caption, charging ? "CHG" : "BAT");
    lv_obj_set_style_text_color(ui.bat_caption, charging ? color : kMuted, 0);
    lv_obj_set_style_img_recolor(ui.bat_icon, color, 0);
    battery::update_fill(ui.bat_fill, percent, charging, status.battery_valid);
}

void StartupView::apply_language() const {
    lv_label_set_text(ui.btn_label, i18n::text(i18n::TextId::StartupStart));
    lv_label_set_text(ui.hold_hint, i18n::text(i18n::TextId::StartupHoldHint));
}
