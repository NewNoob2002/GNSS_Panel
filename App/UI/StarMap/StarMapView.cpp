#include "StarMapView.h"
#include <cstdio>
#include "UI/Common/PageStyle.h"

using namespace page;

namespace {
constexpr lv_color_t kBackground = style::kBackground;
constexpr lv_color_t kDivider = style::kBorder;
constexpr lv_color_t kLabel = style::kMuted;
constexpr lv_color_t kValue = style::kText;
constexpr lv_color_t kActivity = style::kSuccess;

constexpr lv_color_t kColors[7] = {
    LV_COLOR_MAKE(0xE1, 0xAA, 0x22), LV_COLOR_MAKE(0x34, 0x98, 0xDB), LV_COLOR_MAKE(0x2E, 0xCC, 0x71),
    LV_COLOR_MAKE(0xE7, 0x4C, 0x3C), LV_COLOR_MAKE(0x9B, 0x59, 0xB6), LV_COLOR_MAKE(0x1A, 0xBC, 0x9C),
    LV_COLOR_MAKE(0xE6, 0x7E, 0x22),
};

constexpr const char* kNames[7] = {"GPS", "BDS", "GLN", "GAL", "SBA", "QZS", "IRN"};
} // namespace

void StarMapView::create(lv_obj_t* root) {
    lv_obj_set_size(root, 294, 100);
    lv_obj_set_align(root, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_bg_color(root, kBackground, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_outline_width(root, 0, LV_STATE_FOCUSED);
    lv_obj_set_scrollable(root, false);

    const lv_font_t* font_title = resource_pool::get_font("oswaldBold_18");
    const lv_font_t* font_label = resource_pool::get_font("oswaldBold_12");

    lv_obj_t* label_title = lv_label_create(root);
    lv_obj_remove_style_all(label_title);
    lv_obj_set_style_text_font(label_title, font_title, 0);
    lv_obj_set_style_text_color(label_title, kValue, 0);
    lv_obj_set_pos(label_title, 10, 4);
    ui.label_title = label_title;
    auto* demo = style::mark_demo(lv_label_create(root));
    lv_obj_set_style_text_font(demo, font_label, 0);
    lv_obj_set_style_text_color(demo, kLabel, 0);
    lv_label_set_text(demo, "DEMO");
    lv_obj_set_pos(demo, 242, 4);

    lv_obj_t* activity_indicator = lv_obj_create(root);
    lv_obj_remove_style_all(activity_indicator);
    lv_obj_set_size(activity_indicator, 6, 6);
    lv_obj_set_style_radius(activity_indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(activity_indicator, kActivity, 0);
    lv_obj_set_style_bg_opa(activity_indicator, LV_OPA_30, 0);
    ui.activity_indicator = activity_indicator;

    lv_obj_t* divider = lv_obj_create(root);
    lv_obj_remove_style_all(divider);
    lv_obj_set_size(divider, 274, 1);
    lv_obj_set_pos(divider, 10, 30);
    lv_obj_set_style_bg_color(divider, kDivider, 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    ui.divider = divider;

    constexpr int item_width = 39;
    constexpr int item_height = 58;
    constexpr int item_start_x = 10;

    for (int i = 0; i < 7; ++i) {
        lv_obj_t* cont = lv_obj_create(root);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, item_width, item_height);
        lv_obj_set_pos(cont, item_start_x + i * item_width, 36);
        ui.constell[i].cont = cont;

        lv_obj_t* accent = lv_obj_create(cont);
        lv_obj_remove_style_all(accent);
        lv_obj_set_size(accent, 14, 2);
        lv_obj_align(accent, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(accent, kColors[i], 0);
        lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
        ui.constell[i].accent = accent;

        lv_obj_t* label_val = lv_label_create(cont);
        lv_obj_remove_style_all(label_val);
        lv_obj_set_width(label_val, item_width);
        lv_obj_set_style_text_align(label_val, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(label_val, font_title, 0);
        lv_obj_set_style_text_color(label_val, kValue, 0);
        lv_label_set_text(label_val, "0");
        lv_obj_set_pos(label_val, 0, 8);
        ui.constell[i].label_val = label_val;

        lv_obj_t* label_name = lv_label_create(cont);
        lv_obj_remove_style_all(label_name);
        lv_obj_set_width(label_name, item_width);
        lv_obj_set_style_text_align(label_name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(label_name, font_label, 0);
        lv_obj_set_style_text_color(label_name, kLabel, 0);
        lv_label_set_text(label_name, kNames[i]);
        lv_obj_align(label_name, LV_ALIGN_BOTTOM_MID, 0, -1);
        ui.constell[i].label_name = label_name;

        if (i < 6) {
            lv_obj_t* separator = lv_obj_create(cont);
            lv_obj_remove_style_all(separator);
            lv_obj_set_size(separator, 1, 38);
            lv_obj_align(separator, LV_ALIGN_RIGHT_MID, 0, 3);
            lv_obj_set_style_bg_color(separator, kDivider, 0);
            lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
        }
    }

    update(DataProc::DeviceSnapshot{}); // "-" until the platform publishes constellation counts.
    apply_language();
}

void StarMapView::destroy() {}

void StarMapView::apply_language() const {
    lv_label_set_text(ui.label_title, i18n::text(i18n::TextId::StarMapTitle));
    lv_obj_align_to(ui.activity_indicator, ui.label_title, LV_ALIGN_OUT_RIGHT_MID, 7, 0);
}

void StarMapView::update_activity_indicator() {
    activity_indicator_on = !activity_indicator_on;
    lv_obj_set_style_bg_opa(ui.activity_indicator, activity_indicator_on ? LV_OPA_COVER : LV_OPA_30, 0);
}

void StarMapView::update(const DataProc::DeviceSnapshot& device) {
    static_assert(DataProc::kConstellationCount == 7, "one column per constellation");
    char buf[16];
    for (std::size_t i = 0; i < DataProc::kConstellationCount; ++i) {
        if (device.constellations_valid)
            std::snprintf(buf, sizeof(buf), "%u", device.constellations[i]);
        else
            std::snprintf(buf, sizeof(buf), "-");
        lv_label_set_text(ui.constell[i].label_val, buf);
    }
    if (device.constellations_valid)
        update_activity_indicator(); // Blinks while fresh counts arrive.
}
