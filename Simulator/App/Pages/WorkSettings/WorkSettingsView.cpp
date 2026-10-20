#include "WorkSettingsView.h"
#include <cmath>
#include <cstdint>
#include "Utils/PageStyle.h"

using namespace page;

constexpr lv_coord_t font_height = 26;

int8_t WorkSettingsView::left_roller_index = 0;
int8_t WorkSettingsView::right_roller_index = 0;

static void lv_anim_label_set_y(void* obj, const int32_t y) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), y);
}

void WorkSettingsView::create(lv_obj_t* root) {
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    roller_create(root);
    btn_cont_create(root);
}

void WorkSettingsView::destroy() {}

void WorkSettingsView::roller_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 224, 94);
    lv_obj_set_pos(cont, 6, 28);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    page::style::panel(cont);
    page::style::accent(cont, 8, 3, 98);
    page::style::accent(cont, 126, 3, 98);
    page::style::appear(cont);
    ui.roller.cont = cont;

    lv_obj_t* cont_left = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_left);
    page::style::card(cont_left);
    lv_obj_set_size(cont_left, 98, 30);
    lv_obj_set_pos(cont_left, 8, 8);
    lv_obj_clear_flag(cont_left, LV_OBJ_FLAG_SCROLLABLE);
    page::style::appear(cont_left, 50);
    ui.roller.left_roller.cont = cont_left;

    lv_obj_t* label_left = lv_label_create(cont_left);
    lv_obj_set_style_text_font(label_left, resource_pool::get_font("oswaldBold_18"), 0);
    lv_obj_set_style_text_color(label_left, lv_color_white(), 0);
    lv_obj_set_style_text_line_space(label_left, 0, 0);
    lv_obj_set_width(label_left, 96);
    lv_label_set_long_mode(label_left, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_left, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label_left, 1, 0);
    ui.roller.left_roller.label = label_left;

    lv_obj_t* cont_right = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_right);
    page::style::card(cont_right);
    lv_obj_set_size(cont_right, 98, 30);
    lv_obj_set_pos(cont_right, 126, 8);
    lv_obj_clear_flag(cont_right, LV_OBJ_FLAG_SCROLLABLE);
    page::style::appear(cont_right, 90);
    ui.roller.right_roller.cont = cont_right;

    lv_obj_t* label_right = lv_label_create(cont_right);
    lv_obj_set_style_text_font(label_right, resource_pool::get_font("oswaldBold_18"), 0);
    lv_obj_set_style_text_color(label_right, lv_color_white(), 0);
    lv_obj_set_style_text_line_space(label_right, 0, 0);
    lv_obj_set_width(label_right, 96);
    lv_label_set_long_mode(label_right, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_right, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label_right, 1, 0);
    ui.roller.right_roller.label = label_right;

    lv_obj_t* cont_select_left = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_select_left);
    lv_obj_set_size(cont_select_left, 90, 40);
    lv_obj_align_to(cont_select_left, cont_left, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    ui.roller.left_roller.btn_up = btn_create(cont_select_left, resource_pool::get_image("up"), -20, 0);
    ui.roller.left_roller.btn_down = btn_create(cont_select_left, resource_pool::get_image("down"), 20, 0);
    page::style::appear(cont_select_left, 120);

    lv_obj_t* cont_select_right = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_select_right);
    lv_obj_set_size(cont_select_right, 90, 40);
    lv_obj_align_to(cont_select_right, cont_right, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    ui.roller.right_roller.btn_up = btn_create(cont_select_right, resource_pool::get_image("up"), -20, 0);
    ui.roller.right_roller.btn_down = btn_create(cont_select_right, resource_pool::get_image("down"), 20, 0);
    page::style::appear(cont_select_right, 160);

    // 原 reset 控件实际执行返回，不伪装成恢复默认设置。
    ui.roller.btn_reset = btn_create(cont, resource_pool::get_image("back"), 0, 32);
    apply_language();
}

void WorkSettingsView::apply_language() const {
    lv_label_set_text(ui.roller.left_roller.label, i18n::text(i18n::TextId::WorkRadioProtocolOptions));
    lv_label_set_text(ui.roller.right_roller.label, i18n::text(i18n::TextId::WorkRadioChannelOptions));
    roller_to_index(ui.roller.left_roller.label, left_roller_index);
    roller_to_index(ui.roller.right_roller.label, right_roller_index);
}

void WorkSettingsView::scroll(lv_obj_t* label, int delta) {
    if (label != ui.roller.left_roller.label && label != ui.roller.right_roller.label)
        return;
    int8_t& index = label == ui.roller.left_roller.label ? left_roller_index : right_roller_index;
    const int count = label == ui.roller.left_roller.label ? 8 : 10;
    index = static_cast<int8_t>((index + delta + count) % count);
    roller_to_index(label, index);
}

void WorkSettingsView::roller_style_init(lv_obj_t* obj) {
    page::style::control(obj);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xff931e), LV_STATE_FOCUSED);
    lv_obj_update_layout(obj);
}

void WorkSettingsView::btn_cont_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 52, 94);
    lv_obj_set_pos(cont, 236, 28);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    page::style::panel(cont);
    page::style::appear(cont, 120);

    ui.btn_cont.cont = cont;

    ui.btn_cont.btn_base = btn_create(cont, resource_pool::get_image("base"), 0, -30);
    ui.btn_cont.btn_rover = btn_create(cont, resource_pool::get_image("rover"), 0, 0);
    ui.btn_cont.btn_ntrip = btn_create(cont, resource_pool::get_image("ntrip"), 0, 30);
}

lv_obj_t* WorkSettingsView::btn_create(lv_obj_t* par, const void* img_src, const lv_coord_t x_ofs,
                                       const lv_coord_t y_ofs) {
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 35, 26);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, y_ofs);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    roller_style_init(obj);

    return obj;
}

void WorkSettingsView::roller_to_index(lv_obj_t* obj, const uint8_t index) {
    lv_anim_del(obj, lv_anim_label_set_y);

    const lv_coord_t current_y = lv_obj_get_y(obj);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, current_y, -index * font_height);
    lv_anim_set_time(&a, 300);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, lv_anim_label_set_y);
    lv_anim_start(&a);
}
