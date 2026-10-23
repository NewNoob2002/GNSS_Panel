#pragma once
#include "App.h"
#include "Def/DP_Button.h"

namespace page {
/** @brief UI 线程的两键路由与 LVGL 控件分发；不持有同步回调参数。 */
class InputRouter {
  public:
    InputRouter(PageManager& manager, lv_group_t* group, DataNode& account)
        : manager_(manager), group_(group), account_(account) {}
    void on_key(App::Key key, bool pressed);
    void on_key_at(App::Key key, bool pressed, uint64_t event_ms);
    void poll_keys(uint64_t now_ms);
    void receive(const dp::ButtonState& state) { sample_ = state; }
    void cancel_input();
    void cancel_keys();
    /** @brief 平台直接输入保持同步语义，不得从 DataNode 回调中调用。 */
    void on_input(App::InputAction action);
    /** @brief 定时器中消费手势产生的动作，返回本节拍是否已处理。 */
    bool run_pending();
    bool back();
    const char* current_page() const;
    lv_obj_t* focused() const;

  private:
    PageManager& manager_;
    lv_group_t* group_;
    DataNode& account_;
    dp::ButtonState sample_{};
    bool power_hold_ = false;
    const char* key_page_ = nullptr;
    App::InputAction pending_action_ = App::InputAction::None;
    lv_obj_t* pressed_ = nullptr;
    const char* pressed_page_ = nullptr;
    void dispatch_input(App::InputAction action);
};
}
