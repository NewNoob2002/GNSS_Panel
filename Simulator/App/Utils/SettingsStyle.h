#pragma once
#include <cstring>
#include "Resource/ResourcePool.h"
#include "Utils/PageStyle.h"

namespace page::settings_style {
inline lv_obj_t* icon(lv_obj_t* parent, const char* name, int x, int y, lv_color_t color = style::kText) {
    auto* image = lv_img_create(parent);
    lv_img_set_src(image, resource_pool::get_image(name));
    lv_obj_set_pos(image, x, y);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_recolor(image, color, 0);
    lv_obj_set_style_img_recolor_opa(image, LV_OPA_COVER, 0);
    return image;
}
inline void hero(lv_obj_t* obj) {
    style::card(obj);
    lv_obj_set_style_bg_color(obj, style::kPanel, 0);
    lv_obj_set_style_bg_grad_color(obj, style::kCard, 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(obj, 10, 0);
}
inline void move_in(void* obj, int32_t y) {
    lv_obj_set_style_translate_y(static_cast<lv_obj_t*>(obj), static_cast<lv_coord_t>(y), 0);
}
inline void reveal(lv_obj_t* obj, uint32_t delay = 0) {
    lv_anim_del(obj, move_in);
    style::appear(obj, delay);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, obj);
    lv_anim_set_exec_cb(&animation, move_in);
    lv_anim_set_values(&animation, 6, 0);
    lv_anim_set_time(&animation, 240);
    lv_anim_set_delay(&animation, delay);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    if (!lv_anim_start(&animation))
        move_in(obj, 0);
}
inline void value(lv_obj_t* label, const char* text) {
    if (std::strcmp(lv_label_get_text(label), text) == 0)
        return;
    lv_label_set_text(label, text);
    lv_obj_fade_in(label, 140, 0);
}
inline void pulse(void* obj, int32_t opa) {
    lv_obj_set_style_bg_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(opa), 0);
}
inline void stop(lv_obj_t* dot) {
    if (!dot)
        return;
    lv_anim_del(dot, pulse);
    pulse(dot, LV_OPA_COVER);
}
inline void activity(lv_obj_t* dot, bool active, lv_color_t color) {
    lv_obj_set_style_bg_color(dot, color, 0);
    if (!active) {
        stop(dot);
        return;
    }
    if (lv_anim_get(dot, pulse))
        return;
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, dot);
    lv_anim_set_exec_cb(&animation, pulse);
    lv_anim_set_values(&animation, LV_OPA_40, LV_OPA_COVER);
    lv_anim_set_time(&animation, 700);
    lv_anim_set_playback_time(&animation, 700);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    if (!lv_anim_start(&animation))
        pulse(dot, LV_OPA_COVER);
}
inline lv_obj_t* dot(lv_obj_t* parent, int x, int y) {
    auto* obj = style::box(parent, x, y, 6, 6);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    return obj;
}
} // namespace page::settings_style
