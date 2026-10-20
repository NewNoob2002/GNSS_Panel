#pragma once

#include "Utils/Log/Log.h"
#include "Utils/PageStyle.h"

namespace page::battery {

inline lv_color_t color(unsigned percent, bool charging) {
    if (charging || percent >= 50)
        return lv_color_hex(0x28c76f);
    if (percent >= 20)
        return lv_color_hex(0xff931e);
    if (percent >= 10)
        return lv_color_hex(0xea5455);
    return lv_color_hex(0xd03c3b);
}

/** 在 LVGL 线程创建电池槽；返回填充对象，父对象为槽框，width/height 是内部尺寸（px）。 */
inline lv_obj_t* create_slot(lv_obj_t* parent, int x, int y, int width, int height) {
    auto* slot = style::box(parent, x, y, width + 4, height + 4);
    lv_obj_set_style_radius(slot, 2, 0);
    lv_obj_set_style_border_width(slot, 1, 0);
    lv_obj_set_style_bg_color(slot, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(slot, LV_OPA_COVER, 0);
    lv_obj_add_flag(slot, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    auto* fill = style::box(slot, 1, 1, 0, height);
    lv_obj_set_style_radius(fill, 1, 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    auto* terminal = style::box(slot, width + 3, height / 4, 2, height / 2 + 2);
    lv_obj_set_style_radius(terminal, 1, 0);
    lv_obj_set_style_bg_opa(terminal, LV_OPA_COVER, 0);
    return fill;
}

inline void set_width(void* fill, int32_t width) {
    lv_obj_set_width(static_cast<lv_obj_t*>(fill), static_cast<lv_coord_t>(width));
}

/** 在删除槽或离开页面前停止其动画；fill 可为空。 */
inline void stop(lv_obj_t* fill) {
    if (fill)
        lv_anim_del(fill, set_width);
}

/** 在 LVGL 线程更新：充电时 1.2s 从空到满、停留 0.3s 后循环；重复更新不重置动画。
 * 非充电时按实际电量静态显示，越界电量截到 100%；无效数据清空槽并停止动画。 */
inline void update_fill(lv_obj_t* fill, unsigned percent, bool charging, bool valid) {
    percent = percent > 100 ? 100 : percent;
    const auto tint = valid ? color(percent, charging) : lv_color_hex(0x999999);
    auto* slot = lv_obj_get_parent(fill);
    lv_obj_set_style_bg_color(fill, tint, 0);
    lv_obj_set_style_border_color(slot, tint, 0);
    lv_obj_set_style_bg_color(lv_obj_get_child(slot, 1), tint, 0);
    const auto width = lv_obj_get_style_width(slot, 0) - 4;
    if (!valid || !charging) {
        stop(fill);
        set_width(fill, valid ? width * percent / 100 : 0);
        return;
    }
    if (lv_anim_get(fill, set_width))
        return;
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, fill);
    lv_anim_set_exec_cb(&animation, set_width);
    lv_anim_set_values(&animation, 0, width);
    lv_anim_set_time(&animation, 1200);
    lv_anim_set_repeat_delay(&animation, 300);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&animation, lv_anim_path_linear);
    if (!lv_anim_start(&animation)) {
        set_width(fill, width * percent / 100);
        APP_LOG_E("Battery", "charge animation allocation failed; showing actual level");
    }
}

} // namespace page::battery
