//
// Created by guoti on 2025/12/14.
//

#include "numberFlow_clock.h"

NumberFlowClock::NumberFlowClock(const lv_font_t* font)
    : font_(font), cont_(nullptr), hour_(nullptr), minute_(nullptr), second_(nullptr), separator1_(nullptr),
      separator2_(nullptr) {
    // 创建三个数字流组件
    hour_ = new NumberFlow(font, 2);
    minute_ = new NumberFlow(font, 2);
    second_ = new NumberFlow(font, 2);
}

NumberFlowClock::~NumberFlowClock() {
    // 删除数字流组件
    if (hour_) {
        delete hour_;
        hour_ = nullptr;
    }
    if (minute_) {
        delete minute_;
        minute_ = nullptr;
    }
    if (second_) {
        delete second_;
        second_ = nullptr;
    }
    // 删除容器会自动删除所有子对象（包括分隔符）
    if (cont_ && lv_obj_is_valid(cont_)) {
        lv_obj_del(cont_);
        cont_ = nullptr;
    }
}

void NumberFlowClock::create(lv_obj_t* parent) {
    if (parent == nullptr || !lv_obj_is_valid(parent))
        return;
    if (cont_ != nullptr) {
        if (lv_obj_is_valid(cont_))
            return;
        cont_ = nullptr;
        separator1_ = nullptr;
        separator2_ = nullptr;
    }

    const uint16_t font_width = lv_font_get_glyph_width(font_, '0', '\0');
    const lv_coord_t font_height = font_->line_height;
    const uint16_t colon_width = lv_font_get_glyph_width(font_, ':', '\0');

    // 创建主容器
    cont_ = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_);
    lv_obj_clear_flag(cont_, LV_OBJ_FLAG_SCROLLABLE);

    // 计算总宽度：2位数字 + 冒号 + 2位数字 + 冒号 + 2位数字
    const lv_coord_t total_width = font_width * 2 + colon_width + font_width * 2 + colon_width + font_width * 2;
    lv_obj_set_size(cont_, total_width, font_height);

    // 创建小时数字流
    hour_->create(cont_);
    lv_obj_t* hour_cont = hour_->get_cont();
    lv_obj_set_align(hour_cont, LV_ALIGN_TOP_LEFT); // 清除居中对齐
    lv_obj_set_pos(hour_cont, 0, 0);

    // 创建第一个冒号分隔符
    separator1_ = lv_label_create(cont_);
    lv_label_set_text_static(separator1_, ":");
    lv_obj_set_style_text_font(separator1_, font_, 0);
    lv_obj_set_style_text_color(separator1_, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_all(separator1_, 0, 0);
    lv_obj_set_align(separator1_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(separator1_, font_width * 2, 0);

    // 创建分钟数字流
    minute_->create(cont_);
    lv_obj_t* minute_cont = minute_->get_cont();
    lv_obj_set_align(minute_cont, LV_ALIGN_TOP_LEFT); // 清除居中对齐
    lv_obj_set_pos(minute_cont, font_width * 2 + colon_width, 0);

    // 创建第二个冒号分隔符
    separator2_ = lv_label_create(cont_);
    lv_label_set_text_static(separator2_, ":");
    lv_obj_set_style_text_font(separator2_, font_, 0);
    lv_obj_set_style_text_color(separator2_, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_all(separator2_, 0, 0);
    lv_obj_set_align(separator2_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(separator2_, font_width * 2 + colon_width + font_width * 2, 0);

    // 创建秒数字流
    second_->create(cont_);
    lv_obj_t* second_cont = second_->get_cont();
    lv_obj_set_align(second_cont, LV_ALIGN_TOP_LEFT); // 清除居中对齐
    lv_obj_set_pos(second_cont, font_width * 2 + colon_width + font_width * 2 + colon_width, 0);
}

void NumberFlowClock::set_pos(const lv_align_t align, const lv_coord_t x, const lv_coord_t y) const {
    if (cont_ == nullptr || !lv_obj_is_valid(cont_))
        return;
    lv_obj_align(cont_, align, x, y);
}

void NumberFlowClock::set_time(const uint32_t hour_val, const uint32_t minute_val, const uint32_t second_val) const {
    if (cont_ == nullptr || !lv_obj_is_valid(cont_))
        return;
    if (hour_) {
        hour_->set_value(hour_val);
    }
    if (minute_) {
        minute_->set_value(minute_val);
    }
    if (second_) {
        second_->set_value(second_val);
    }
}
