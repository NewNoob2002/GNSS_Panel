#ifndef LVGL_NUMBER_FLOW_H
#define LVGL_NUMBER_FLOW_H

#include <vector>
#include "lvgl.h"

class NumberFlow {
  public:
    explicit NumberFlow(const lv_font_t* default_font, const int number_size, const bool hide = false)
        : cont_(nullptr), font_(default_font), number_size_(number_size), last_value_(-1),
          hidden_high_position_(hide) {};

    ~NumberFlow() {
        // 删除容器会自动删除所有子对象，不需要单独删除 digit_labels 中的指针
        if (this->cont_ && lv_obj_is_valid(this->cont_)) {
            lv_obj_del(this->cont_);
        }
        digit_labels_.clear();
    }

    void create(lv_obj_t* parent);

    void set_pos(lv_align_t align, lv_coord_t x, lv_coord_t y) const;

    void set_align_to(const lv_obj_t* base, lv_align_t align, lv_coord_t x, lv_coord_t y) const;

    void set_value(uint32_t target_value) const;

    // 获取容器指针，用于布局
    lv_obj_t* get_cont() const {
        return cont_;
    }

  private:
    lv_obj_t* cont_;
    const lv_font_t* font_;
    uint16_t number_size_;
    std::vector<lv_obj_t*> digit_labels_;
    mutable uint32_t last_value_;               // 记录上一次的值，避免重复动画
    mutable std::vector<uint32_t> last_digits_; // 记录每个数字位的上一次值
    bool hidden_high_position_;
    // 内部辅助函数：执行动画
    void animate_digit(uint32_t digit_index, uint32_t target_val) const;
    // 内部辅助函数：隐藏/显示数字位
    void set_digit_visibility(uint32_t digit_index, bool visible) const;
};

#endif //LVGL_NUMBER_FLOW_H
