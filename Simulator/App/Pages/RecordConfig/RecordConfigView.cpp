//
// Created by gtc on 2026/5/9.
//

#include "RecordConfigView.h"
#include <cmath>
#include "Utils/PageStyle.h"

using namespace page;

constexpr lv_coord_t font_height = 26;

int8_t RecordConfigView::left_roller_index = 0;
int8_t RecordConfigView::right_roller_index = 0;

static void lv_anim_label_set_y(void* obj, const int32_t y) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), y);
}

void RecordConfigView::create(lv_obj_t* root) {
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    roller_create(root);
    btn_cont_create(root);
}

void RecordConfigView::destroy() {}

void RecordConfigView::roller_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 224, 94);
    lv_obj_set_pos(cont, 6, 28);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    page::style::panel(cont);
    page::style::accent(cont, 28, 20, 72);
    page::style::accent(cont, 136, 20, 82);
    page::style::appear(cont);
    ui.roller.cont = cont;

    lv_obj_t* img_left = lv_img_create(cont);
    lv_img_set_src(img_left, resource_pool::get_image("mode"));
    lv_obj_set_pos(img_left, 8, 34);

    lv_obj_t* cont_left = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_left);
    page::style::card(cont_left);
    lv_obj_set_size(cont_left, 72, 30);
    lv_obj_set_pos(cont_left, 28, 27);
    lv_obj_clear_flag(cont_left, LV_OBJ_FLAG_SCROLLABLE);
    page::style::appear(cont_left, 50);
    ui.roller.left_roller.cont = cont_left;

    lv_obj_t* label_left = lv_label_create(cont_left);
    lv_obj_set_style_text_font(label_left, resource_pool::get_font("oswaldBold_18"), 0);
    lv_obj_set_style_text_color(label_left, lv_color_white(), 0);
    lv_obj_set_style_text_line_space(label_left, 0, 0);
    lv_obj_set_width(label_left, 70);
    lv_label_set_long_mode(label_left, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_left, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label_left, 1, 0);
    ui.roller.left_roller.label = label_left;

    lv_obj_t* img_right = lv_img_create(cont);
    lv_img_set_src(img_right, resource_pool::get_image("clock"));

    lv_obj_t* cont_right = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_right);
    page::style::card(cont_right);
    lv_obj_set_size(cont_right, 82, 30);
    lv_obj_set_pos(cont_right, 136, 27);
    lv_obj_clear_flag(cont_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(img_right, 118, 34);
    page::style::appear(cont_right, 90);
    ui.roller.right_roller.cont = cont_right;

    lv_obj_t* label_right = lv_label_create(cont_right);
    lv_obj_set_style_text_font(label_right, resource_pool::get_font("oswaldBold_18"), 0);
    lv_obj_set_style_text_color(label_right, lv_color_white(), 0);
    lv_obj_set_style_text_line_space(label_right, 0, 0);
    lv_obj_set_width(label_right, 80);
    lv_label_set_long_mode(label_right, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_right, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label_right, 1, 0);
    ui.roller.right_roller.label = label_right;

    lv_obj_t* cont_select_left = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_select_left);
    // 35px buttons at +-20 keep the 5px gap used by WorkSettings; the row is centered under the 72px card.
    lv_obj_set_size(cont_select_left, 76, 40);
    lv_obj_set_pos(cont_select_left, 26, 57);

    ui.roller.left_roller.btn_up = btn_create(cont_select_left, resource_pool::get_image("up"), -20, 0);
    ui.roller.left_roller.btn_down = btn_create(cont_select_left, resource_pool::get_image("down"), 20, 0);
    page::style::appear(cont_select_left, 120);

    lv_obj_t* cont_select_right = lv_obj_create(cont);
    lv_obj_remove_style_all(cont_select_right);
    lv_obj_set_size(cont_select_right, 82, 40);
    lv_obj_set_pos(cont_select_right, 136, 57);

    ui.roller.right_roller.btn_up = btn_create(cont_select_right, resource_pool::get_image("up"), -20, 0);
    ui.roller.right_roller.btn_down = btn_create(cont_select_right, resource_pool::get_image("down"), 20, 0);
    page::style::appear(cont_select_right, 160);

    apply_language();
}

void RecordConfigView::apply_language() const {
    lv_label_set_text(ui.roller.left_roller.label, i18n::text(i18n::TextId::RecordTypeOptions));
    lv_label_set_text(ui.roller.right_roller.label, i18n::text(i18n::TextId::RecordIntervalOptions));
    roller_to_index(ui.roller.left_roller.label, left_roller_index);
    roller_to_index(ui.roller.right_roller.label, right_roller_index);
}

void RecordConfigView::scroll(lv_obj_t* label, int delta) {
    if (label != ui.roller.left_roller.label && label != ui.roller.right_roller.label)
        return;
    int8_t& index = label == ui.roller.left_roller.label ? left_roller_index : right_roller_index;
    const int count = label == ui.roller.left_roller.label ? 2 : 6;
    index = static_cast<int8_t>((index + delta + count) % count);
    roller_to_index(label, index);
}

void RecordConfigView::roller_style_init(lv_obj_t* obj) {
    page::style::control(obj);
    lv_obj_set_style_width(obj, 45, LV_STATE_PRESSED);
    lv_obj_set_style_height(obj, 25, LV_STATE_PRESSED);
    lv_obj_update_layout(obj);
}

void RecordConfigView::btn_cont_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 52, 94);
    lv_obj_set_pos(cont, 236, 28);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    page::style::panel(cont);
    page::style::appear(cont, 120);

    ui.btn_cont.cont = cont;

    ui.btn_cont.btn_record = btn_create(cont, resource_pool::get_image("start"), 0, -20);
    ui.btn_cont.btn_return = btn_create(cont, resource_pool::get_image("back"), 0, 22);
}

lv_obj_t* RecordConfigView::btn_create(lv_obj_t* par, const void* img_src, const lv_coord_t x_ofs,
                                       const lv_coord_t y_ofs) {
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 35, 26);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, y_ofs);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    page::style::control(obj);
    lv_obj_update_layout(obj);

    return obj;
}

void RecordConfigView::roller_to_index(lv_obj_t* obj, const uint8_t index) {
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
