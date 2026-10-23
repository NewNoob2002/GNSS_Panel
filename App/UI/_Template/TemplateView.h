#pragma once

#include "lvgl.h"

namespace page {
/** @brief View 独占控件写入；Presenter 只通过 getter 转发输入与管理生命周期。 */
class TemplateView {
  public:
    void create(lv_obj_t* root);
    /** @brief 普通 observer 显式管理；bind 立即渲染初值，destroy/离页必须先 unbind。 */
    bool bind(lv_subject_t* subject);
    void unbind();
    void destroy();
    lv_obj_t* action_control() const { return label_; }

  private:
    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;
    lv_obj_t* label_ = nullptr;
};
} // namespace page
