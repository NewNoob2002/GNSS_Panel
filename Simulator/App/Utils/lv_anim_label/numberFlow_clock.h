//
// Created by guoti on 2025/12/14.
//

#ifndef LVGL_NUMBERFLOW_CLOCK_H
#define LVGL_NUMBERFLOW_CLOCK_H

#include "numberFlow.h"

class NumberFlowClock {
  public:
    explicit NumberFlowClock(const lv_font_t* font);

    ~NumberFlowClock();

    void create(lv_obj_t* parent);

    void set_pos(lv_align_t align, lv_coord_t x, lv_coord_t y) const;

    void set_time(uint32_t hour_val, uint32_t minute_val, uint32_t second_val) const;

    [[nodiscard]] lv_obj_t* get_cont() const {
        return cont_;
    };

  private:
    const lv_font_t* font_;
    lv_obj_t* cont_;       // 主容器
    NumberFlow* hour_;     // 小时（2位）
    NumberFlow* minute_;   // 分钟（2位）
    NumberFlow* second_;   // 秒（2位）
    lv_obj_t* separator1_; // 第一个冒号
    lv_obj_t* separator2_; // 第二个冒号
};

#endif //LVGL_NUMBERFLOW_CLOCK_H
