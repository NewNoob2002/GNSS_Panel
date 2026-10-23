#include "InputRouter.h"
#include <cstring>

using namespace page;
using InputAction = App::InputAction;

void InputRouter::on_input(const InputAction action) {
    if (!group_ || manager_.isSwitching())
        return;
    cancel_keys();
    dispatch_input(action);
}

void InputRouter::dispatch_input(const InputAction action) {
    auto* current = manager_.getCurrentPage();
    if (!current)
        return;
    if (action == InputAction::Press && pressed_ == focused() && pressed_page_ == current_page())
        return;
    if (action == InputAction::NextFocus && current->onNextRequest())
        return;
    if (action == InputAction::Confirm && current->onEnterRequest())
        return;
    if (action == InputAction::Commit) {
        current->onCommitRequest();
        return;
    }
    const char* page = current_page();
    if (action == InputAction::Release) {
        lv_obj_t* pressed = pressed_;
        const char* pressed_page = pressed_page_;
        pressed_ = nullptr;
        pressed_page_ = nullptr;
        if (pressed && page && std::strcmp(page, pressed_page) == 0 && lv_obj_is_in_widget_tree(pressed)) {
            lv_obj_set_pressed(pressed, false);
            lv_obj_send_event(pressed, LV_EVENT_RELEASED, nullptr);
            if (lv_group_get_focused(group_) == pressed && !current->onEnterRequest())
                lv_obj_send_event(pressed, LV_EVENT_SHORT_CLICKED, nullptr);
        }
        return;
    }
    if (pressed_ && action != InputAction::None) {
        if (page && std::strcmp(page, pressed_page_) == 0 && lv_obj_is_in_widget_tree(pressed_)) {
            lv_obj_set_pressed(pressed_, false);
            lv_obj_send_event(pressed_, LV_EVENT_PRESS_LOST, nullptr);
        }
        pressed_ = nullptr;
        pressed_page_ = nullptr;
    }
    if (action == InputAction::NextFocus)
        lv_group_focus_next(group_);
    else if (action == InputAction::PreviousFocus)
        lv_group_focus_prev(group_);
    else if (action == InputAction::Confirm || action == InputAction::Press) {
        if (lv_obj_t* focused = lv_group_get_focused(group_)) {
            if (action == InputAction::Press && page) {
                pressed_ = focused;
                /** @brief 删除通知指向成员；同一控件只保留一份，避免自动重复积累事件。 */
                for (uint32_t i = lv_obj_get_event_count(focused); i > 0; --i) {
                    auto* event = lv_obj_get_event_dsc(focused, i - 1);
                    if (lv_event_dsc_get_user_data(event) == &pressed_)
                        lv_obj_remove_event(focused, i - 1);
                }
                lv_obj_null_on_delete(&pressed_);
                pressed_page_ = page;
                lv_obj_set_pressed(focused, true);
                lv_obj_send_event(focused, LV_EVENT_PRESSED, nullptr);
            } else {
                lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, nullptr);
            }
        }
    } else if (action == InputAction::Back) {
        back();
    }
}

void InputRouter::cancel_input() {
    (void)dp::Button::notify(account_, {dp::ButtonOperation::Cancel, dp::ButtonKey::Both, false, true});
    key_page_ = current_page();
    pending_action_ = InputAction::None;
    auto* pressed = pressed_;
    const auto* page = pressed_page_;
    /** @brief 指针按压由 LVGL 持有，单独 reset 不会发出 PRESS_LOST。 */
    if (auto* focus = focused(); focus && lv_obj_is_pressed(focus)) {
        pressed = focus;
        page = current_page();
    }
    pressed_ = nullptr;
    pressed_page_ = nullptr;
    if (pressed && page && current_page() && std::strcmp(page, current_page()) == 0 && lv_obj_is_in_widget_tree(pressed)) {
        lv_obj_set_pressed(pressed, false);
        lv_obj_send_event(pressed, LV_EVENT_PRESS_LOST, nullptr);
    }
}

void InputRouter::cancel_keys() {
    (void)dp::Button::notify(account_, {dp::ButtonOperation::Cancel});
    key_page_ = current_page();
    pending_action_ = InputAction::None;
}

void InputRouter::on_key(App::Key key, bool pressed) {
    (void)dp::Button::notify(account_, {dp::ButtonOperation::Level,
        key == App::Key::Power ? dp::ButtonKey::Power : dp::ButtonKey::Function, pressed});
}

void InputRouter::on_key_at(App::Key key, bool pressed, uint64_t event_ms) {
    poll_keys(event_ms);
    on_key(key, pressed);
    poll_keys(event_ms);
}

void InputRouter::poll_keys(uint64_t now_ms) {
    if (manager_.isSwitching() || key_page_ != current_page()) {
        /** @brief 切页时仍推进节点时钟，保持旧时间戳不会重新启动去抖。 */
        cancel_keys();
        (void)dp::Button::notify(account_, {dp::ButtonOperation::Poll, dp::ButtonKey::Both, false, false, now_ms});
        return;
    }
    if (dp::Button::notify(account_, {dp::ButtonOperation::Poll, dp::ButtonKey::Both, false, false, now_ms}) != DataNode::RES_OK)
        return;
    const auto sample = sample_;
    auto* current = manager_.getCurrentPage();
    const bool hold = current && current->wantsPowerHold();
    if (hold != power_hold_) {
        power_hold_ = hold;
        (void)dp::Button::notify(account_, {dp::ButtonOperation::Cancel, dp::ButtonKey::Power});
        return;
    }
    if (hold) {
        /** @brief 回调已返回后再分发电平；冷启动 Power 已按住时仍须计时。 */
        dispatch_input(sample.power_held ? InputAction::Press : InputAction::Release);
        if (sample.function != ButtonGesture::Action::None && !sample.power_held)
            pending_action_ = sample.function == ButtonGesture::Action::Single ? InputAction::NextFocus : InputAction::Back;
        return;
    }
    if (sample.power != ButtonGesture::Action::None)
        pending_action_ = sample.power == ButtonGesture::Action::Single ? InputAction::Confirm : InputAction::Commit;
    else if (sample.function != ButtonGesture::Action::None)
        pending_action_ = sample.function == ButtonGesture::Action::Single ? InputAction::NextFocus : InputAction::Back;
}

bool InputRouter::run_pending() {
    if (pending_action_ == InputAction::None)
        return false;
    const auto action = pending_action_;
    pending_action_ = InputAction::None;
    if (key_page_ == current_page())
        on_input(action);
    return true;
}

const char* InputRouter::current_page() const {
    auto* current = manager_.getCurrentPage();
    return current ? current->getName() : nullptr;
}

bool InputRouter::back() {
    if (manager_.isSwitching())
        return false;
    cancel_keys();
    if (auto* current = manager_.getCurrentPage(); current && current->onBackRequest())
        return true;
    return manager_.pop() == PageManager::RES_TYPE::OK;
}

lv_obj_t* InputRouter::focused() const {
    return group_ ? lv_group_get_focused(group_) : nullptr;
}
