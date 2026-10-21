#include "SystemDash.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
using namespace page;

void SystemDash::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_OVER_BOTTOM, 280, lv_anim_path_ease_out);
}
void SystemDash::on_view_load() {
    if (!model_.init())
        app_fatal("SystemDash", "Model initialization failed");
    view_.create(root, model_.settings().reboot_available);
    utils::attach_controls(view_.all_controls(), clicked, this);
    for (auto* action : {view_.ui.shutdown, view_.ui.reboot})
        lv_obj_add_event_cb(action, hold_event, LV_EVENT_ALL, this);
}
void SystemDash::on_view_will_appear() {
    model_.set_status_bar(false);
    view_.apply_language();
    view_.update(model_.settings());
    model_.set_change_callback([this] {
        view_.update(model_.settings());
    });
    utils::focus_controls(view_.controls(), last_focus_);
    view_.appear();
}
void SystemDash::on_view_will_disappear() {
    view_.stop();
    model_.set_change_callback({});
    cancel_hold();
    last_focus_ = utils::leave_focus_group();
    if (view_.power_open()) {
        // Leaving (SaveConfig or a platform shutdown) ends the confirmation; return lands on the Power row.
        view_.show_power(false);
        last_focus_ = view_.ui.power;
    }
}
void SystemDash::on_view_unload() {
    cancel_hold();
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}
void SystemDash::on_language_changed() {
    view_.apply_language();
    view_.update(model_.settings());
    if (priv.state != PAGE_STATE_ACTIVITY)
        view_.stop();
}
void SystemDash::show_power(bool visible) {
    cancel_hold();
    lv_group_remove_all_objs(lv_group_get_default());
    view_.show_power(visible);
    utils::focus_controls(view_.controls(), visible ? view_.ui.shutdown : view_.ui.power);
}
bool SystemDash::on_back_request() {
    if (!view_.power_open())
        return false;
    show_power(false);
    return true;
}
void SystemDash::cancel_hold() {
    if (hold_timer_) {
        lv_timer_del(hold_timer_);
        hold_timer_ = nullptr;
    }
    if (holding_)
        lv_anim_del(this, set_hold_progress);
    holding_ = nullptr;
    view_.set_hold(nullptr, 0);
}
void SystemDash::set_hold_progress(void* self, int32_t percent) {
    auto* page = static_cast<SystemDash*>(self);
    page->view_.set_hold(page->holding_, percent);
}
// Press on Shutdown/Reboot starts the bar; releasing before it fills (or losing the press/focus) closes the
// dialog. Only a completed hold prepares the power action.
void SystemDash::hold_event(lv_event_t* event) {
    auto* self = static_cast<SystemDash*>(lv_event_get_user_data(event));
    auto* target = lv_event_get_current_target(event);
    const auto code = lv_event_get_code(event);
    if (!self->view_.power_open() || self->page_manager->is_switching())
        return;
    if (code == LV_EVENT_PRESSED) {
        if (self->holding_)
            return;
        self->holding_ = target;
        self->hold_timer_ = lv_timer_create(on_hold_complete, kHoldMs, self);
        if (!self->hold_timer_) {
            APP_LOG_E("SystemDash", "power hold timer allocation failed");
            self->show_power(false);
            return;
        }
        lv_timer_set_repeat_count(self->hold_timer_, 1);
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, self);
        lv_anim_set_exec_cb(&anim, set_hold_progress);
        lv_anim_set_values(&anim, 0, 100);
        lv_anim_set_time(&anim, kHoldMs);
        if (!lv_anim_start(&anim))
            APP_LOG_W("SystemDash", "power hold animation allocation failed; confirmation still times the hold");
    } else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED)
               && self->holding_ == target) {
        self->show_power(false);
    }
}
void SystemDash::on_hold_complete(lv_timer_t* timer) {
    auto* self = static_cast<SystemDash*>(timer->user_data);
    self->hold_timer_ = nullptr; // A one-shot timer deletes itself after this callback.
    const auto action =
        self->holding_ == self->view_.ui.reboot ? DataProc::PowerAction::Reboot : DataProc::PowerAction::Off;
    self->view_.set_hold(self->holding_, 100);
    lv_anim_del(self, set_hold_progress);
    if (!self->model_.prepare_power(action)) {
        APP_LOG_E("SystemDash", "power preparation rejected");
        self->show_power(false);
        return;
    }
    if (!self->page_manager->push("Pages/SaveConfig")) {
        if (!self->model_.cancel_power())
            APP_LOG_E("SystemDash", "power rollback failed");
        self->show_power(false);
    }
}
void SystemDash::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemDash*>(lv_event_get_user_data(event));
    if (self->page_manager->is_switching())
        return;
    auto* target = lv_event_get_current_target(event);
    if (self->view_.power_open()) {
        self->show_power(false); // A short press never executes shutdown/reboot; it dismisses the dialog.
        return;
    }
    if (target == self->view_.ui.power)
        self->show_power(true);
    else if (target == self->view_.ui.language) {
        if (!self->model_.toggle_language())
            APP_LOG_E("SystemDash", "language request rejected");
    } else if (target == self->view_.ui.wifi) {
        const auto state = self->model_.settings();
        if (state.wifi_available && !state.wifi_pending && !self->model_.set_wifi(!state.wifi_on))
            APP_LOG_E("SystemDash", "Wi-Fi request rejected");
    } else if (target == self->view_.ui.back && !self->page_manager->pop())
        APP_LOG_E("SystemDash", "Return navigation rejected");
}
