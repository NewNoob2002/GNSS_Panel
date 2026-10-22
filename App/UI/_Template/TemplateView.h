#pragma once

#include "lvgl.h"

namespace page {
/** 仅拥有控件引用，控件由 PageManager 的页面根对象统一销毁。 */
class TemplateView {
  public:
    /** 在页面根对象下创建控件。 */
    void create(lv_obj_t* root);
    /** 根对象卸载时清除引用。 */
    void destroy();

  private:
    lv_obj_t* label_ = nullptr;
};
} // namespace page
