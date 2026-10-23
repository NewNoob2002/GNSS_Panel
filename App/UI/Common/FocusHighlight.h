/** @file FocusHighlight.h @brief 卡片焦点的透明覆盖边框，限 LVGL 应用线程使用。 */
#pragma once
#include <array>
#include <cstddef>
#include "Utils/Spring/lv_spring.h"

namespace page {
/** @brief 借用 card/control 控件；先于控件或后于控件销毁均安全，空闲与离页无驱动定时器。 */
class FocusHighlight {
  public:
    ~FocusHighlight() { unbind(); }
    FocusHighlight() = default;
    FocusHighlight(const FocusHighlight&) = delete;
    FocusHighlight& operator=(const FocusHighlight&) = delete;

    /** @brief 显式接管最多六个 card/control 的焦点边框；首次聚焦直接落位，失败撤销绑定。 */
    template <std::size_t N> bool bind(lv_obj_t* root, const std::array<lv_obj_t*, N>& controls) {
        static_assert(N <= 6, "焦点控件超出页面容量");
        return bind_controls(root, controls.data(), N);
    }
    /** @brief 恢复 card/control 样式、解除所有事件并删除覆盖框，不删除控件。 */
    void unbind();
    /** @brief 首次进入及恢复时对齐当前焦点，再允许后续保速重定向。 */
    void resume();
    /** @brief 语言或布局变化直接落位；不启动定时器。 */
    void sync();
    /** @brief 离页停止推进，保留过渡所需画面，恢复前新焦点直接落位。 */
    void stop();
    lv_obj_t* object() const { return motion_.object(); }
    const app::LvSpring& motion() const { return motion_; }

  private:
    bool bind_controls(lv_obj_t* root, lv_obj_t* const* controls, std::size_t count);
    void focus_to(lv_obj_t* object, bool animated, bool layout_only = false);
    void watch_parent(lv_obj_t* parent);
    void visibility();
    static void on_control(lv_event_t* event);
    static void on_parent(lv_event_t* event);
    app::LvSpring motion_{};
    std::array<lv_obj_t*, 6> controls_{};
    lv_obj_t *focused_ = nullptr, *parent_ = nullptr;
    bool animate_ = false, aligning_ = false;
};
} // namespace page
