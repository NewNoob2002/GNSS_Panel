#include "numberFlow.h"

// 动画回调函数
static void lv_anim_label_set_y(void* obj, const int32_t y) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), y);
}

void NumberFlow::create(lv_obj_t* parent) {
    if (parent == nullptr || !lv_obj_is_valid(parent))
        return;
    if (cont_ != nullptr) {
        if (lv_obj_is_valid(cont_))
            return;
        cont_ = nullptr;
        digit_labels_.clear();
        last_digits_.clear();
        last_value_ = static_cast<uint32_t>(-1);
    }

    const uint16_t font_width = lv_font_get_glyph_width(font_, '0', '\0');
    const lv_coord_t font_height = font_->line_height;

    // 创建容器并保存到成员变量
    cont_ = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_);
    lv_obj_center(cont_);
    lv_obj_set_size(cont_, font_width * number_size_, font_height);
    lv_obj_clear_flag(cont_, LV_OBJ_FLAG_SCROLLABLE);       // 禁用滚动
    lv_obj_clear_flag(cont_, LV_OBJ_FLAG_OVERFLOW_VISIBLE); // 确保裁剪子对象

    for (int i = 0; i < number_size_; i++) {
        // 为每个数字位创建容器
        lv_obj_t* digit_cont = lv_obj_create(cont_);
        lv_obj_remove_style_all(digit_cont);
        lv_obj_set_size(digit_cont, font_width, font_height);
        lv_obj_set_pos(digit_cont, i * font_width, 0);
        lv_obj_clear_flag(digit_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(digit_cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        // 创建包含所有数字的长条 label
        lv_obj_t* label = lv_label_create(digit_cont);
        lv_label_set_text(label, "0\n1\n2\n3\n4\n5\n6\n7\n8\n9");
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP); // 设置长文本模式为裁剪
        lv_obj_set_style_text_font(label, font_, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_pad_all(label, 0, 0);
        lv_obj_set_style_text_line_space(label, 0, 0); // 关键：设置行间距为0

        // 设置 label 的宽度和高度（高度为所有数字的总高度：10行）
        lv_obj_set_width(label, font_width);
        lv_obj_set_height(label, font_height * 10);

        // 设置初始位置，显示第一个0（只使用 set_pos，不使用 align）
        lv_obj_set_pos(label, 0, 0);

        digit_labels_.push_back(label);
    }
}

void NumberFlow::set_pos(const lv_align_t align, const lv_coord_t x, const lv_coord_t y) const {
    if (cont_ == nullptr || !lv_obj_is_valid(cont_))
        return;
    lv_obj_align(cont_, align, x, y);
}

void NumberFlow::set_align_to(const lv_obj_t* base, const lv_align_t align, const lv_coord_t x,
                              const lv_coord_t y) const {
    if (cont_ == nullptr || !lv_obj_is_valid(cont_) || base == nullptr || !lv_obj_is_valid(base))
        return;
    lv_obj_align_to(cont_, base, align, x, y);
}

// 内部辅助：只针对需要改变的位启动动画
void NumberFlow::animate_digit(const uint32_t digit_index, const uint32_t target_val) const {
    if (digit_index >= digit_labels_.size())
        return;
    if (digit_labels_[digit_index] == nullptr || !lv_obj_is_valid(digit_labels_[digit_index]))
        return;

    lv_obj_t* label = digit_labels_[digit_index];
    const lv_coord_t font_height = font_->line_height;

    // 先删除该 label 上可能存在的旧动画，避免冲突
    lv_anim_del(label, lv_anim_label_set_y);

    // 删除动画后，获取当前实际位置
    const lv_coord_t current_y = lv_obj_get_y(label);
    const lv_coord_t target_y = -(target_val * font_height);

    // 如果已经在目标位置，直接返回
    if (current_y == target_y)
        return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_values(&a, current_y, target_y);
    lv_anim_set_time(&a, 300);                      // 缩短动画时间到300ms，避免卡顿
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out); // 缓动效果
    lv_anim_set_exec_cb(&a, lv_anim_label_set_y);
    lv_anim_start(&a);
}

// 内部辅助：隐藏/显示数字位
void NumberFlow::set_digit_visibility(const uint32_t digit_index, const bool visible) const {
    if (digit_index >= digit_labels_.size())
        return;
    if (digit_labels_[digit_index] == nullptr || !lv_obj_is_valid(digit_labels_[digit_index]))
        return;

    // 获取数字位的容器（label 的父对象）
    lv_obj_t* digit_cont = lv_obj_get_parent(digit_labels_[digit_index]);
    if (digit_cont == nullptr || !lv_obj_is_valid(digit_cont))
        return;

    if (visible) {
        lv_obj_clear_flag(digit_cont, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(digit_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

void NumberFlow::set_value(const uint32_t target_value) const {
    if (cont_ == nullptr || !lv_obj_is_valid(cont_) || digit_labels_.empty())
        return;

    // 如果值没有改变，直接返回，避免频繁启动动画
    if (target_value == last_value_)
        return;
    last_value_ = target_value;

    // 初始化 last_digits_ 向量
    if (last_digits_.size() != number_size_) {
        last_digits_.resize(number_size_, -1);
    }

    // 根据位数拆分数字，只对改变的数字位启动动画
    switch (number_size_) {
        case 1: {
            if (const uint32_t units = target_value % 10; last_digits_[0] != units) {
                animate_digit(0, units);
                last_digits_[0] = units;
            }
            break;
        }
        case 2: {
            // digit_labels_[0] 是左边的（十位）
            // digit_labels_[1] 是右边的（个位）
            const uint32_t tens = (target_value / 10) % 10;
            const uint32_t units = target_value % 10;

            if (hidden_high_position_) {
                // 如果十位是0，隐藏十位；否则显示十位
                if (tens == 0) {
                    set_digit_visibility(0, false);
                } else {
                    set_digit_visibility(0, true);
                    if (last_digits_[0] != tens) {
                        animate_digit(0, tens);
                        last_digits_[0] = tens;
                    }
                }
            } else {
                // 正常显示所有位
                set_digit_visibility(0, true);
                if (last_digits_[0] != tens) {
                    animate_digit(0, tens);
                    last_digits_[0] = tens;
                }
            }

            // 个位始终更新
            if (last_digits_[1] != units) {
                animate_digit(1, units);
                last_digits_[1] = units;
            }
            break;
        }
        case 3: {
            const uint32_t hundreds = (target_value / 100) % 10;
            const uint32_t tens = (target_value / 10) % 10;
            const uint32_t units = target_value % 10;

            if (hidden_high_position_) {
                // 如果百位是0，隐藏百位；否则显示百位
                if (hundreds == 0) {
                    set_digit_visibility(0, false);
                    // 如果百位和十位都是0，也隐藏十位
                    if (tens == 0) {
                        set_digit_visibility(1, false);
                    } else {
                        set_digit_visibility(1, true);
                        if (last_digits_[1] != tens) {
                            animate_digit(1, tens);
                            last_digits_[1] = tens;
                        }
                    }
                } else {
                    // 百位不为0，显示百位和十位
                    set_digit_visibility(0, true);
                    set_digit_visibility(1, true);
                    if (last_digits_[0] != hundreds) {
                        animate_digit(0, hundreds);
                        last_digits_[0] = hundreds;
                    }
                    if (last_digits_[1] != tens) {
                        animate_digit(1, tens);
                        last_digits_[1] = tens;
                    }
                }
            } else {
                if (last_digits_[0] != hundreds) {
                    animate_digit(0, hundreds);
                    last_digits_[0] = hundreds;
                }
                if (last_digits_[1] != tens) {
                    animate_digit(1, tens);
                    last_digits_[1] = tens;
                }
            }

            // 个位始终更新
            if (last_digits_[2] != units) {
                animate_digit(2, units);
                last_digits_[2] = units;
            }
            break;
        }
        default:;
    }
}
