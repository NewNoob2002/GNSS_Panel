#pragma once

#include "Log.h"
#include "UI/Common/PageStyle.h"

namespace page::battery {

/** @brief 10–19% 低电量用的浅红，介于橙色告警与危险红之间，仅电池使用。 */
inline constexpr lv_color_t kLow = LV_COLOR_MAKE(0xEA, 0x54, 0x55);

inline lv_color_t color(unsigned percent, bool charging) {
    if (charging || percent >= 50)
        return style::kSuccess;
    if (percent >= 20)
        return style::kAccent;
    if (percent >= 10)
        return kLow;
    return style::kDanger;
}

/** 在 LVGL 线程创建电池槽；返回填充对象，父对象为槽框，width/height 是内部尺寸（px）。 */
inline lv_obj_t* create_slot(lv_obj_t* parent, int x, int y, int width, int height) {
    auto* slot = style::box(parent, x, y, width + 4, height + 4);
    lv_obj_set_style_radius(slot, 2, 0);
    lv_obj_set_style_border_width(slot, 1, 0);
    lv_obj_set_style_bg_color(slot, style::kBorder, 0);
    lv_obj_set_style_bg_opa(slot, LV_OPA_COVER, 0);
    lv_obj_set_overflow_visible(slot, true);
    // v9 需显式扩展绘制区域，电池正极超出槽框右缘 2 px。
    lv_obj_set_ext_draw_size(slot, 2);
    lv_obj_refresh_ext_draw_size(slot);
    auto* fill = style::box(slot, 1, 1, 0, height);
    lv_obj_set_style_radius(fill, 1, 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    auto* terminal = style::box(slot, width + 3, height / 4, 2, height / 2 + 2);
    lv_obj_set_style_radius(terminal, 1, 0);
    lv_obj_set_style_bg_opa(terminal, LV_OPA_COVER, 0);
    return fill;
}

inline void set_width(void* fill, int32_t width) {
    lv_obj_set_width(static_cast<lv_obj_t*>(fill), static_cast<int32_t>(width));
}

/** 在删除槽或离开页面前停止其动画；fill 可为空。 */
inline void stop(lv_obj_t* fill) {
    if (fill)
        lv_anim_delete(fill, set_width);
}

/** 充电动画起点：当前电量宽度，最多为槽宽的 4/5，接近满电时仍保留可见的扫动。 */
inline int32_t charge_start(int32_t width, unsigned percent) {
    const int32_t level = width * static_cast<int32_t>(percent) / 100;
    const int32_t cap = width * 4 / 5;
    return level < cap ? level : cap;
}

/** 在 LVGL 线程更新：充电时 1.2s 从当前电量扫到满、停留 0.3s 后循环，电量变化后从新起点重启；
 * 非充电时按实际电量静态显示，越界电量截到 100%；无效数据清空槽并停止动画。 */
inline void update_fill(lv_obj_t* fill, unsigned percent, bool charging, bool valid) {
    percent = percent > 100 ? 100 : percent;
    const auto tint = valid ? color(percent, charging) : style::kMuted;
    auto* slot = lv_obj_get_parent(fill);
    lv_obj_set_style_bg_color(fill, tint, 0);
    lv_obj_set_style_border_color(slot, tint, 0);
    lv_obj_set_style_bg_color(lv_obj_get_child(slot, 1), tint, 0);
    const auto width = lv_obj_get_style_width(slot, LV_PART_MAIN) - 4;
    if (!valid || !charging) {
        stop(fill);
        set_width(fill, valid ? width * percent / 100 : 0);
        return;
    }
    const int32_t start = charge_start(width, percent);
    if (const auto* running = lv_anim_get(fill, set_width)) {
        if (running->start_value == start)
            return; // 重复更新不重置动画。
        stop(fill);
    }
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, fill);
    lv_anim_set_exec_cb(&animation, set_width);
    lv_anim_set_values(&animation, start, width);
    lv_anim_set_duration(&animation, 1200);
    lv_anim_set_repeat_delay(&animation, 300);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&animation, lv_anim_path_linear);
    if (!lv_anim_start(&animation)) {
        set_width(fill, width * percent / 100);
        APP_LOG_E("Battery", "charge animation allocation failed; showing actual level");
    }
}

} // namespace page::battery
