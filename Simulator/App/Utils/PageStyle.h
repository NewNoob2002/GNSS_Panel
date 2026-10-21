#ifndef APP_PAGE_STYLE_H
#define APP_PAGE_STYLE_H

#include "lvgl.h"

namespace page::style {

inline constexpr lv_color_t kBackground = LV_COLOR_MAKE(0x10, 0x12, 0x14);
inline constexpr lv_color_t kPanel = LV_COLOR_MAKE(0x20, 0x24, 0x28);
inline constexpr lv_color_t kCard = LV_COLOR_MAKE(0x18, 0x1C, 0x20);
inline constexpr lv_color_t kBorder = LV_COLOR_MAKE(0x41, 0x4A, 0x52);
inline constexpr lv_color_t kText = LV_COLOR_MAKE(0xF2, 0xF4, 0xF5);
inline constexpr lv_color_t kMuted = LV_COLOR_MAKE(0xA4, 0xAD, 0xB5);
inline constexpr lv_color_t kAccent = LV_COLOR_MAKE(0xFF, 0x93, 0x1E);
inline constexpr lv_color_t kHighlight = LV_COLOR_MAKE(0xF6, 0xD0, 0x62);
inline constexpr lv_color_t kFocus = LV_COLOR_MAKE(0x34, 0x30, 0x2A);
inline constexpr lv_color_t kPressed = LV_COLOR_MAKE(0x3A, 0x42, 0x4A);
inline constexpr lv_color_t kSuccess = LV_COLOR_MAKE(0x28, 0xC7, 0x6F);
inline constexpr lv_color_t kInfo = LV_COLOR_MAKE(0x34, 0x98, 0xDB);
inline constexpr lv_color_t kDanger = LV_COLOR_MAKE(0xD0, 0x3C, 0x3B);

inline lv_obj_t* box(lv_obj_t* parent, int x, int y, int width, int height, bool clickable = false) {
    auto* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    if (clickable)
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    return obj;
}
inline lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, int x, int y, int width) {
    auto* obj = lv_label_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, width);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, kText, 0);
    return obj;
}

inline void panel(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, kPanel, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 12, 0);
}

inline void card(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, kCard, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, kBorder, 0);
    lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_border_width(obj, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(obj, kAccent, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(obj, kFocus, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(obj, kPressed, LV_STATE_PRESSED);
}

inline lv_obj_t* accent(lv_obj_t* parent, const lv_coord_t x, const lv_coord_t y, const lv_coord_t width) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(obj, width, 3);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_bg_color(obj, kAccent, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 2, 0);
    return obj;
}

inline void appear(lv_obj_t* obj, const uint32_t delay_ms = 0) {
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(obj, 180, delay_ms);
}

inline void control(lv_obj_t* obj, const lv_color_t pressed_color = kPressed) {
    card(obj);
    lv_obj_set_style_bg_color(obj, pressed_color, LV_STATE_PRESSED);
    lv_obj_set_style_opa(obj, LV_OPA_50, LV_STATE_DISABLED);

    static lv_style_transition_dsc_t transition;
    static constexpr lv_style_prop_t properties[] = {LV_STYLE_WIDTH, LV_STYLE_HEIGHT, LV_STYLE_PROP_INV};
    static bool initialized = false;
    if (!initialized) {
        lv_style_transition_dsc_init(&transition, properties, lv_anim_path_ease_out, 200, 0, nullptr);
        initialized = true;
    }
    lv_obj_set_style_transition(obj, &transition, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &transition, LV_STATE_FOCUSED);
}

/** DEMO markers: only shown when the platform reports simulated data (SystemState::simulated). */
constexpr lv_obj_flag_t kDemoMarker = LV_OBJ_FLAG_USER_1;
inline lv_obj_t* mark_demo(lv_obj_t* obj) {
    lv_obj_add_flag(obj, kDemoMarker);
    return obj;
}
/** Show or hide every DEMO marker below root, after the view has been created. */
inline void show_demo(lv_obj_t* root, bool simulated) {
    if (!root)
        return;
    if (lv_obj_has_flag(root, kDemoMarker)) {
        if (simulated)
            lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    }
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(root); ++i)
        show_demo(lv_obj_get_child(root, static_cast<int32_t>(i)), simulated);
}

} // namespace page::style

#endif
