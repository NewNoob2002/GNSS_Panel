#ifndef APP_PAGE_STYLE_H
#define APP_PAGE_STYLE_H

#include "lvgl.h"

namespace page::style {

/** 切屏只平移不透明页面根，不用整页透明度。
 * 开机过渡用 FADE_THROUGH 单色遮罩，无离屏图层。 */
inline constexpr uint16_t kPageTransitionMs = 350;

/** @brief 页面切换曲线 cubic-bezier(0.2, 0, 0, 1)：起步柔和、长减速收尾，替代起步即最快的 ease-out。 */
inline int32_t page_transition_path(const lv_anim_t* a) {
    const auto t = static_cast<int32_t>(lv_map(a->act_time, 0, a->duration, 0, LV_BEZIER_VAL_MAX));
    const int32_t step = lv_cubic_bezier(t, LV_BEZIER_VAL_FLOAT(0.2), 0, 0, LV_BEZIER_VAL_MAX);
    return a->start_value + ((step * (a->end_value - a->start_value)) >> LV_BEZIER_VAL_SHIFT);
}

/** @brief 切屏时压暗被覆盖页的遮罩最深不透明度（PUSH_*、OVER_*）；单色混合、无离屏图层，开销远低于整页淡出。 */
inline constexpr lv_opa_t kPageScrimOpa = 115; // 约 45%

/**
 * @brief 首页入口面板展开/收回曲线 cubic-bezier(0.4, 0, 0.2, 1)（标准缓入缓出），时长同 kPageTransitionMs。
 *        展开不像整页平移那样有内容随尾段移动，强缓出曲线会在前约 1/3 时长盖满、随后留下一段空白等待。
 */
inline int32_t panel_transition_path(const lv_anim_t* a) {
    const auto t = static_cast<int32_t>(lv_map(a->act_time, 0, a->duration, 0, LV_BEZIER_VAL_MAX));
    const int32_t step =
        lv_cubic_bezier(t, LV_BEZIER_VAL_FLOAT(0.4), 0, LV_BEZIER_VAL_FLOAT(0.2), LV_BEZIER_VAL_MAX);
    return a->start_value + ((step * (a->end_value - a->start_value)) >> LV_BEZIER_VAL_SHIFT);
}

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
    lv_obj_set_scrollable(obj, false); lv_obj_set_clickable(obj, false);
    if (clickable)
        lv_obj_set_clickable(obj, true);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    return obj;
}
inline lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, int x, int y, int width) {
    auto* obj = lv_label_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, width);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, kText, 0);
    return obj;
}

/** Create an opaque full-circle progress arc starting at twelve o'clock without input handling. */
inline lv_obj_t* ring(lv_obj_t* parent, int x, int y, int size, int width, lv_color_t color) {
    auto* obj = lv_arc_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_clickable(obj, false); lv_obj_set_scrollable(obj, false);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, size, size);
    lv_arc_set_bg_angles(obj, 0, 360);
    lv_arc_set_rotation(obj, 270);
    lv_arc_set_range(obj, 0, 100);
    lv_arc_set_value(obj, 0);
    lv_obj_set_style_arc_width(obj, width, LV_PART_MAIN);
    lv_obj_set_style_arc_width(obj, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(obj, kPanel, LV_PART_MAIN);
    lv_obj_set_style_arc_color(obj, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(obj, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(obj, LV_OPA_COVER, LV_PART_INDICATOR);
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

inline lv_obj_t* accent(lv_obj_t* parent, const int32_t x, const int32_t y, const int32_t width) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_scrollable(obj, false);
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
constexpr uint32_t kDemoMarker = 0;
inline lv_obj_t* mark_demo(lv_obj_t* obj) {
    lv_obj_set_user_flag(obj, kDemoMarker, true);
    return obj;
}
/** Show or hide every DEMO marker below root, after the view has been created. */
inline void show_demo(lv_obj_t* root, bool simulated) {
    if (!root)
        return;
    if (lv_obj_get_user_flag(root, kDemoMarker)) {
        if (simulated)
            lv_obj_set_hidden(root, false);
        else
            lv_obj_set_hidden(root, true);
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i)
        show_demo(lv_obj_get_child(root, static_cast<int32_t>(i)), simulated);
}

} // namespace page::style

#endif
